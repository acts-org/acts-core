#!/bin/bash

set -e

# helper function to selectively print and run commands without a subshell
function run() {
    set -x
    "$@"
    # save exit code
    { rec=$?; } 2> /dev/null
    { set +x;   } 2> /dev/null
    # restore exit code
    (exit $rec)
}

export run


shopt -s extglob


mode=${1:-all}
if ! [[ $mode = @(all|kalman|gsf|gx2f|fullchains|vertexing|simulation|traccc_host|traccc_cuda) ]]; then
    echo "Usage: $0 <all|kalman|gsf|gx2f|fullchains|vertexing|simulation|traccc_host|traccc_cuda> (outdir)"
    exit 1
fi

outdir=${2:-physmon}
mkdir -p $outdir

refdir=CI/physmon/reference
refcommit=$(cat $refdir/commit)
commit=$(git rev-parse --short HEAD)
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}"  )" &> /dev/null && pwd  )

# File to accumulate the histcmp results
histcmp_results=$outdir/histcmp_results.csv
echo -n "" > $histcmp_results

memory_dir=${outdir}/memory
mkdir -p "$memory_dir"

if [ "$(uname)" == "Darwin" ]; then
    function measure {
        label=$1
        slug=$2
        shift
        shift
        echo "Measure Darwin $label ($slug)" >&2
        tmp=$(mktemp)
        echo "+ $@" >&2
        /usr/bin/time -l -o "$tmp" "$@"
        # save exit code
        rec=$?

        of="${memory_dir}/mem_${slug}.csv"
        {
            echo "# spyral-label: $label"
            echo "# spyral-cmd: $*"
            echo "time,rss,vms"
            # in bytes
            grep -E "real" "$tmp" | awk '{printf $1}'
            printf ","
            grep -E "maximum resident set size" "$tmp" | awk '{printf $1}'
            printf ",0\n"
        } > "$of"
        # restore exit code
        (exit $rec)
    }
    export measure
elif [ "$(uname)" == "Linux" ]; then
    function measure {
        label=$1
        slug=$2
        shift
        shift
        echo "Measure Linux $label ($slug)" >&2
        tmp=$(mktemp)
        echo "+ $@" >&2
        /usr/bin/time -v -o  "$tmp" "$@"
        # save exit code
        rec=$?
        # in kbytes
        max_rss=$(grep "Maximum resident set size (kbytes):" "$tmp" | awk '{printf $(NF)}')
        max_rss=$(( 1000*max_rss ))
        wall_time=$(grep "Elapsed (wall clock)" "$tmp" | awk '{printf $(NF)}')
        echo $max_rss
        wall_time=$(python3 -c "i='${wall_time}';p=i.split(':');p = p if len(p) == 3 else ['0', *p];t=float(p[0])*60*60 + float(p[1])*60 + float(p[2]);print(t)")
        echo $wall_time

        of="${memory_dir}/mem_${slug}.csv"
        {
            echo "# spyral-label: $label"
            echo "# spyral-cmd: $*"
            echo "time,rss,vms"
            echo "${wall_time},${max_rss},0"
        } > "$of"
        # restore exit code
        (exit $rec)
    }
    export measure
else
    function measure {
        echo "Not measuring because unknown environment"
        shift
        shift
        "$@"
    }
    export measure
fi

if [ -n "$CI" ]; then
    echo "CI mode, do not abort immediately on failure"
    set +e
fi
ec=0

source $SCRIPT_DIR/setup.sh

function run_physmon_gen() {
    title=$1
    slug=$2

    script=CI/physmon/workflows/physmon_${slug}.py

    measure "$title" "$slug" ${script} $outdir 2>&1 > $outdir/run_${slug}.log

    this_ec=$?
    ec=$(($ec | $this_ec))

    if [ $this_ec -ne 0 ]; then
      echo "::error::🟥 Dataset generation failed: ${script} -> ec=$this_ec"
    else
      echo "::notice::✅ Dataset generation succeeded: ${script}"
    fi
}

echo "::group::Generate validation dataset"
if [[ "$mode" == "all" || "$mode" == "kalman" ]]; then
    run_physmon_gen "Truth Tracking KF" "truth_tracking_kalman"
fi
if [[ "$mode" == "all" || "$mode" == "gsf" ]]; then
    run_physmon_gen "Truth Tracking GSF" "truth_tracking_gsf"
fi
if [[ "$mode" == "all" || "$mode" == "gx2f" ]]; then
    run_physmon_gen "Truth Tracking GX2F" "truth_tracking_gx2f"
fi
if [[ "$mode" == "all" || "$mode" == "fullchains" ]]; then
    run_physmon_gen "CKF Tracking" "ckf_tracking"
    run_physmon_gen "Track finding ttbar" "track_finding_ttbar"

fi
if [[ "$mode" == "all" || "$mode" == "vertexing" ]]; then
    run_physmon_gen "Vertexing" "vertexing"
fi
if [[ "$mode" == "all" || "$mode" == "simulation" ]]; then
    run_physmon_gen "Simulation" "simulation"
fi
if [[ "$mode" == "all" || "$mode" == "traccc_host" ]]; then
    run_physmon_gen "Traccc Host Chain" "traccc_chain_host"
fi
if [[ "$mode" == "traccc_cuda" ]]; then
    run_physmon_gen "Traccc Cuda Chain" "traccc_chain_cuda"
fi
echo "::endgroup::"


function run_histcmp() {
    a=$1
    b=$2
    title=$3
    slug=$4
    shift 4

    echo "::group::Comparing $a vs. $b"

    if [ ! -f "$a" ]; then
      echo "::error::histcmp failed: File $a does not exist"
      ec=1
    fi

    if [ ! -f "$b" ]; then
      echo "::error::histcmp failed: File $b does not exist"
      ec=1
    fi

    run histcmp $a $b \
        --label-reference=reference \
        --label-monitored=monitored \
        --title="$title" \
        -o $outdir/${slug}.html \
        -p $outdir/${slug}_plots \
        "$@"

    this_ec=$?
    ec=$(($ec | $this_ec))

    if [ $this_ec -ne 0 ]; then
      echo "::error::histcmp failed (${slug}): ec=$this_ec"
    fi

    echo "\"${title}\",${slug},${this_ec}" >> $histcmp_results

    echo "::endgroup::"
}

function full_chain() {
    suffix=$1

    config="CI/physmon/ckf_${suffix}.yml"

    if [ ! -f "$config" ]; then
        config="CI/physmon/default.yml"
    fi
    echo $config

    if [ $suffix != truth_smeared ]; then
      run_histcmp \
        $outdir/performance_seeding_${suffix}.root \
        $refdir/performance_seeding_${suffix}.root \
        "Seeding ${suffix}" \
        seeding_${suffix} \
        -c $config
    fi

    run_histcmp \
        $outdir/performance_ckf_${suffix}.root \
        $refdir/performance_ckf_${suffix}.root \
        "CKF ${suffix}" \
        ckf_${suffix} \
        -c $config

    run Examples/Scripts/generic_plotter.py \
        $outdir/performance_ivf_${suffix}.root \
        vertexing \
        $outdir/performance_ivf_${suffix}_hist.root \
        --silent \
        --config CI/physmon/vertexing_config.yml
    ec=$(($ec | $?))

    # remove ntuple file because it's large
    rm $outdir/performance_ivf_${suffix}.root

    run_histcmp \
        $outdir/performance_ivf_${suffix}_hist.root \
        $refdir/performance_ivf_${suffix}_hist.root \
        "IVF ${suffix}" \
        ivf_${suffix}

    run Examples/Scripts/generic_plotter.py \
        $outdir/performance_amvf_${suffix}.root \
        vertexing \
        $outdir/performance_amvf_${suffix}_hist.root \
        --silent \
        --config CI/physmon/vertexing_config.yml
    ec=$(($ec | $?))

    # remove ntuple file because it's large
    rm $outdir/performance_amvf_${suffix}.root

    run_histcmp \
        $outdir/performance_amvf_${suffix}_hist.root \
        $refdir/performance_amvf_${suffix}_hist.root \
        "AMVF ${suffix}" \
        amvf_${suffix}

    if [ $suffix == seeded ]; then
	    run Examples/Scripts/generic_plotter.py \
            $outdir/performance_amvf_gridseeder_${suffix}.root \
            vertexing \
            $outdir/performance_amvf_gridseeder_${suffix}_hist.root \
            --silent \
            --config CI/physmon/vertexing_config.yml
        ec=$(($ec | $?))

        # remove ntuple file because it's large
        rm $outdir/performance_amvf_gridseeder_${suffix}.root

        run_histcmp \
            $outdir/performance_amvf_gridseeder_${suffix}_hist.root \
            $refdir/performance_amvf_gridseeder_${suffix}_hist.root \
            "AMVF (+grid seeder) ${suffix}" \
            amvf_gridseeder_${suffix}
    fi

    run Examples/Scripts/generic_plotter.py \
        $outdir/tracksummary_ckf_${suffix}.root \
        tracksummary \
        $outdir/tracksummary_ckf_${suffix}_hist.root \
        --silent \
        --config CI/physmon/tracksummary_ckf_config.yml
    ec=$(($ec | $?))

    # remove ntuple file because it's large
    rm $outdir/tracksummary_ckf_${suffix}.root

    run_histcmp \
        $outdir/tracksummary_ckf_${suffix}_hist.root \
        $refdir/tracksummary_ckf_${suffix}_hist.root \
        "Track Summary CKF ${suffix}" \
        tracksummary_ckf_${suffix}

}

function simulation() {
    suffix=$1

    config="CI/physmon/simulation_config.yml"

    run Examples/Scripts/generic_plotter.py \
        $outdir/particles_${suffix}.root \
        particles \
        $outdir/particles_${suffix}_hist.root \
        --silent \
        --config $config
    ec=$(($ec | $?))

    # remove ntuple file because it's large
    rm $outdir/particles_${suffix}.root

    run_histcmp \
        $outdir/particles_${suffix}_hist.root \
        $refdir/particles_${suffix}_hist.root \
        "Particles ${suffix}" \
        particles_${suffix}
}

if [[ "$mode" == "all" || "$mode" == "fullchains" ]]; then
    full_chain truth_smeared
    full_chain truth_estimated
    full_chain seeded
    full_chain orthogonal

    run_histcmp \
        $outdir/performance_ambi_seeded.root \
        $refdir/performance_ambi_seeded.root \
        "Ambisolver seeded" \
        ambi_seeded

    run_histcmp \
        $outdir/performance_ambi_orthogonal.root \
        $refdir/performance_ambi_orthogonal.root \
        "Ambisolver orthogonal" \
        ambi_orthogonal

    run_histcmp \
        $outdir/performance_seeding_ttbar.root \
        $refdir/performance_seeding_ttbar.root \
        "Seeding ttbar" \
        seeding_ttbar \
        -c $config

    run_histcmp \
        $outdir/performance_ckf_ttbar.root \
        $refdir/performance_ckf_ttbar.root \
        "CKF ttbar" \
        ckf_ttbar \
        -c $config

    run_histcmp \
        $outdir/performance_ambi_ttbar.root \
        $refdir/performance_ambi_ttbar.root \
        "Ambisolver " \
        ambi_ttbar

    run Examples/Scripts/generic_plotter.py \
        $outdir/performance_amvf_ttbar.root \
        vertexing \
        $outdir/performance_amvf_ttbar_hist.root \
        --silent \
        --config CI/physmon/vertexing_ttbar_config.yml
    ec=$(($ec | $?))

    run Examples/Scripts/generic_plotter.py \
        $outdir/tracksummary_ckf_ttbar.root \
        tracksummary \
        $outdir/tracksummary_ckf_ttbar_hist.root \
        --config CI/physmon/tracksummary_ckf_config.yml
    ec=$(($ec | $?))

    # remove ntuple file because it's large
    rm $outdir/tracksummary_ckf_ttbar.root

    run_histcmp \
        $outdir/tracksummary_ckf_ttbar_hist.root \
        $refdir/tracksummary_ckf_ttbar_hist.root \
        "Track Summary CKF ttbar" \
        tracksummary_ckf_ttbar

    # remove ntuple file because it's large
    rm $outdir/performance_amvf_ttbar.root

    run_histcmp \
        $outdir/performance_amvf_ttbar_hist.root \
        $refdir/performance_amvf_ttbar_hist.root \
        "AMVF ttbar" \
        amvf_ttbar

    run Examples/Scripts/generic_plotter.py \
        $outdir/performance_amvf_gridseeder_ttbar.root \
        vertexing \
        $outdir/performance_amvf_gridseeder_ttbar_hist.root \
        --silent \
        --config CI/physmon/vertexing_ttbar_config.yml
    ec=$(($ec | $?))

    # remove ntuple file because it's large
    rm $outdir/performance_amvf_gridseeder_ttbar.root

    run_histcmp \
        $outdir/performance_amvf_gridseeder_ttbar_hist.root \
        $refdir/performance_amvf_gridseeder_ttbar_hist.root \
        "AMVF (+grid seeder) ttbar" \
        amvf_gridseeder_ttbar

    run Examples/Scripts/generic_plotter.py \
        $outdir/pythia8_particles_ttbar.root \
        particles \
        $outdir/particles_ttbar_hist.root \
        --silent \
        --config CI/physmon/pythia8_ttbar_config.yml

    run_histcmp \
      $outdir/particles_ttbar_hist.root \
      $refdir/particles_ttbar_hist.root \
      "Particles ttbar" \
      particles_ttbar

    run Examples/Scripts/generic_plotter.py \
        $outdir/pythia8_vertices_ttbar.root \
        vertices \
        $outdir/vertices_ttbar_hist.root \
        --silent \
        --config CI/physmon/pythia8_ttbar_config.yml

    run_histcmp \
      $outdir/vertices_ttbar_hist.root \
      $refdir/vertices_ttbar_hist.root \
      "Vertices ttbar" \
      vertices_ttbar
fi

if [[ "$mode" == "all" || "$mode" == "traccc" ]]; then
    run Examples/Scripts/generic_plotter.py \
        $outdir/tracksummary_traccc.root \
        tracksummary \
        $outdir/tracksummary_traccc_hist.root \
        --silent \
        --config CI/physmon/traccc_config.yml
    ec=$(($ec | $?))

    rm $outdir/tracksummary_traccc.root

    run_histcmp \
        $outdir/tracksummary_traccc_hist.root \
        $refdir/tracksummary_traccc_hist.root \
        "Track Summary Traccc Chain" \
        tracksummary_traccc
fi

if [[ "$mode" == "all" || "$mode" == "gsf" ]]; then
    run_histcmp \
        $outdir/performance_gsf.root \
        $refdir/performance_gsf.root \
        "Truth tracking (GSF)" \
        gsf \
        -c CI/physmon/gsf.yml
fi

if [[ "$mode" == "all" || "$mode" == "kalman" ]]; then
    run_histcmp \
        $outdir/performance_truth_tracking.root \
        $refdir/performance_truth_tracking.root \
        "Truth tracking" \
        truth_tracking \
        -c CI/physmon/truth_tracking.yml
fi

if [[ "$mode" == "all" || "$mode" == "gx2f" ]]; then
    run_histcmp \
        $outdir/performance_gx2f.root \
        $refdir/performance_gx2f.root \
        "Truth tracking (GX2F)" \
        gx2f \
        -c CI/physmon/gx2f.yml
fi

if [[ "$mode" == "all" || "$mode" == "vertexing" ]]; then
    run Examples/Scripts/vertex_mu_scan.py \
        $outdir/performance_vertexing_*mu*.root \
        $outdir/vertexing_mu_scan.pdf

    rm $outdir/performance_vertexing_*mu*
fi

if [[ "$mode" == "all" || "$mode" == "simulation" ]]; then
    simulation fatras
    simulation geant4
fi

run CI/physmon/summary.py $histcmp_results \
  --md $outdir/summary.md \
  --html $outdir/summary.html
ec=$(($ec | $?))

exit $ec
