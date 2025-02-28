set(ACTS_ACTSVG_SOURCE
    "URL;https://github.com/acts-project/actsvg/archive/refs/tags/v${_acts_actsvg_version}.tar.gz;URL_HASH;SHA256=8073a371465ce2edef3bfdba5eae900e17ee61ae21766111ad1834e29250f304"
    CACHE STRING
    "Source to take ACTSVG from"
)
mark_as_advanced(ACTS_ACTSVG_SOURCE)

set(ACTS_VECMEM_SOURCE
    "URL;https://github.com/acts-project/vecmem/archive/refs/tags/v${_acts_vecmem_version}.tar.gz;URL_HASH;SHA256=fc21cea04140e1210c83a32579b0a7194601889b6c895404214ac55ce547342b"
    CACHE STRING
    "Source to take VECMEM from"
)
mark_as_advanced(ACTS_VECMEM_SOURCE)

set(ACTS_ALGEBRAPLUGINS_SOURCE
    "URL;https://github.com/acts-project/algebra-plugins/archive/refs/tags/v${_acts_algebraplugins_version}.tar.gz;URL_HASH;SHA256=0170f22e1a75493b86464f27991117bc2c5a9d52554c75786e321d4c591990e7"
    CACHE STRING
    "Source to take ALGEBRAPLUGINS from"
)
mark_as_advanced(ACTS_ALGEBRAPLUGINS_SOURCE)

set(ACTS_COVFIE_SOURCE
    "URL;https://github.com/acts-project/covfie/archive/refs/tags/v${_acts_covfie_version}.tar.gz;URL_HASH;SHA256=39fcd0f218d3b4f3aacc6af497a8cda8767511efae7a72b47781f10fd4340f4f"
    CACHE STRING
    "Source to take COVFIE from"
)
mark_as_advanced(ACTS_COVFIE_SOURCE)

set(ACTS_DETRAY_SOURCE
    "URL;https://github.com/acts-project/detray/archive/refs/tags/v${_acts_detray_version}.tar.gz;URL_HASH;SHA256=2d4a76432dd6ddbfc00b88b5d482072e471fefc264b60748bb1f9a123963576e"
    CACHE STRING
    "Source to take DETRAY from"
)
mark_as_advanced(ACTS_DETRAY_SOURCE)

set(ACTS_TRACCC_SOURCE
    "URL;https://github.com/acts-project/traccc/archive/refs/tags/v${_acts_traccc_version}.tar.gz;URL_HASH;SHA256=0a0f923dd4da9b3e76c634264aadf3994bbfe790c0bf4965140e671aa3fd4e2d"
    CACHE STRING
    "Source to take TRACCC from"
)
mark_as_advanced(ACTS_TRACCC_SOURCE)

set(ACTS_FRNN_SOURCE
    "GIT_REPOSITORY;https://github.com/hrzhao76/FRNN/;GIT_TAG;5f8a48b0022300cd2863119f5646a5f31373e0c8"
    CACHE STRING
    "Source to take FRNN from"
)
mark_as_advanced(ACTS_FRNN_SOURCE)

set(ACTS_NLOHMANNJSON_SOURCE
    "URL;https://github.com/nlohmann/json/archive/refs/tags/v${_acts_nlohmanjson_version}.tar.gz;URL_HASH;SHA256=5daca6ca216495edf89d167f808d1d03c4a4d929cef7da5e10f135ae1540c7e4"
    CACHE STRING
    "Source to take nlohmann_json from"
)
mark_as_advanced(ACTS_NLOHMANN_JSON_SOURCE)

set(ACTS_EIGEN3_SOURCE
    "URL;https://gitlab.com/libeigen/eigen/-/archive/${_acts_eigen3_version}/${_acts_eigen3_version}.tar.gz;URL_HASH;SHA256=ba6ef66ba2d319e0a871a267889411c550d4bdf5bc7c62f86c60276913f3f4ba"
    CACHE STRING
    "Source to take eigen3 from"
)
mark_as_advanced(ACTS_EIGEN3_SOURCE)

set(ACTS_PYBIND11_SOURCE
    "GIT_REPOSITORY;https://github.com/pybind/pybind11.git;GIT_TAG;v${_acts_pybind11_version}"
    CACHE STRING
    "Source to take pybind11 from"
)
mark_as_advanced(ACTS_PYBIND11_SOURCE)

set(ACTS_ANNOY_SOURCE
    "URL;https://github.com/spotify/annoy/archive/refs/tags/v${_acts_annoy_version}.tar.gz;URL_HASH;SHA256=c121d38cacd98f5103b24ca4e94ca097f18179eed3037e9eb93ad70ec1e6356e"
    CACHE STRING
    "Source to take Annoy from"
)
mark_as_advanced(ACTS_ANNOY_SOURCE)
