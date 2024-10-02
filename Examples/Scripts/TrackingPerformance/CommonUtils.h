// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <functional>
#include <limits>

#include <TColor.h>
#include <TDirectory.h>
#include <TH1F.h>
#include <TString.h>

/// Helper function:
/// function to set up the histogram style
///
/// @tparam hist_t the histogram type
///
/// @param hist the histogram
/// @param color the color
template <typename hist_t>
void setHistStyle(hist_t* hist, short color = 1) {
  hist->GetXaxis()->SetTitleSize(0.04);
  hist->GetYaxis()->SetTitleSize(0.04);
  hist->GetXaxis()->SetLabelSize(0.04);
  hist->GetYaxis()->SetLabelSize(0.04);
  hist->GetXaxis()->SetTitleOffset(1.0);
  hist->GetYaxis()->SetTitleOffset(1.0);
  hist->GetXaxis()->SetNdivisions(505);
  hist->SetMarkerStyle(20);
  hist->SetMarkerSize(0.8);
  hist->SetLineWidth(2);
  hist->SetTitle("");
  hist->SetLineColor(color);
  hist->SetMarkerColor(color);
}

/// Helper function:
/// function to set up the efficiency histogram style
///
/// @tparam eff_t the efficiency histogram type
///
/// @param eff the efficiency histogram
/// @param color the color to be set
template <typename eff_t>
void setEffStyle(eff_t* eff, short color = 1) {
  eff->SetMarkerStyle(20);
  eff->SetMarkerSize(0.8);
  eff->SetLineWidth(2);
  eff->SetLineColor(color);
  eff->SetMarkerColor(color);
}

/// Helper function: set color palette
///
/// @tparam type of the histogram
///
/// @param h the histogram in question
/// @param rmin the range min value
/// @param rmax the range max value
/// @param rgood the good value of the mistogram
/// @param rwindow the window around the good value to be declared good
/// @param n the number of divisions
template <typename hist_t>
void adaptColorPalette(hist_t* h, float rmin, float rmax, float rgood,
                       float rwindow, int n) {
  // min - max is the range of the axis
  float rel_good = (rgood - rmin) / (rmax - rmin);
  float rel_window = rwindow / (rmax - rmin);

  // Stops are
  const int number = 5;
  double red[number] = {0., 0., 0., 1., 1.};
  double green[number] = {0., 1., 1., 1., 0.};
  double blue[number] = {1., 1., 0., 0., 0.};
  double stops[number] = {0., rel_good - rel_window, rel_good,
                          rel_good + rel_window, 1.};
  h->SetContour(n);

  TColor::CreateGradientColorTable(number, stops, red, green, blue, n);
}

/// Helper function:
/// increase eff range by a scale factor. Note that it assumes the eff has
/// already been drawn
///
/// @tparam eff_t the efficiency histogram type
///
/// @param eff the efficiency histogram
/// @param minScale the minimum of the scale
/// @param maxScale the maximum of the scale
template <typename eff_t>
void adaptEffRange(eff_t* eff, float minScale = 1, float maxScale = 1.1) {
  gPad->Update();
  auto ymin = gPad->GetUymin();
  auto ymax = gPad->GetUymax();
  auto graph = eff->GetPaintedGraph();
  graph->SetMinimum(ymin * minScale);
  graph->SetMaximum(ymax * maxScale);
  gPad->Modified();
  gPad->Update();
}

/// A Parameter handle struct to deal with
/// residuals and pulls.
///
/// This struct allows to define accessors and
/// cuts for residual and pull analysis in order
/// to be able to access them in an ROOT event loop
struct ResidualPullHandle {
  /// A tag name
  std::string tag = "";

  /// Title and names: residual
  std::string residualStr = "";
  std::string residualUnit = "";

  /// Title and names: error
  std::string errorStr = "";

  /// The rangeDrawStr draw string
  std::string rangeDrawStr = "";
  std::string rangeMaxStr = "";
  std::string rangeCutStr = "";

  /// The range array
  std::array<float, 2> range = {0., 0.};

  /// Value function that allows to create
  /// combined parameters
  std::function<float(ULong64_t)> value;

  /// The associated error accessor
  std::function<float(ULong64_t)> error;

  /// The acceptance
  std::function<bool(ULong64_t)> accept;

  TH1F* rangeHist = nullptr;

  TH1F* residualHist = nullptr;

  TH1F* pullHist = nullptr;

  ULong64_t accepted = 0;

  /// Fill the entry
  ///
  /// @param entry is the current TTree entry to be processed
  void fill(unsigned int entry) {
    if (accept(entry)) {
      // Access the value, error
      float v = value(entry);
      residualHist->Fill(v);
      pullHist->Fill(v / error(entry));
      // Count the accessor
      ++accepted;
    }
  };
};

/// This is a s
struct SingleHandle {
  /// A tag name
  std::string tag = "";

  /// A label name
  std::string label = "";

  // Range draw string
  std::string rangeDrawStr = "";

  /// The number of bins for the booking
  unsigned int bins = 1;

  /// The range array
  std::array<float, 2> range = {0., 0.};

  /// Value function that allows to create
  /// combined parameters
  std::function<float(ULong64_t)> value;

  /// The acceptance
  std::function<bool(ULong64_t)> accept;

  TH1F* hist = nullptr;

  /// Fill the entry
  ///
  /// @param entry is the current TTree entry to be processed
  void fill(unsigned int entry) {
    if (accept(entry)) {
      // Access the value, error
      float v = value(entry);
      hist->Fill(v);
    }
  }
};

/// This is a combined accept struct
///
/// It allows to define muleiple accept struct in a chained way
struct AcceptCombination {
  std::function<bool(ULong64_t)> one;

  std::function<bool(ULong64_t)> two;

  /// returns true if value is within range
  /// @param entry the entry in the tree
  bool operator()(ULong64_t entry) { return (one(entry) && two(entry)); }
};

/// This Struct is to accept all values - a placeholder
struct AcceptAll {
  // Call operator always returns true
  bool operator()(ULong64_t /*event*/) { return true; }
};

/// This Struct is to accept a certain range from a
/// TTree accessible value
struct AcceptRange {
  std::vector<float>* value = nullptr;

  std::array<float, 2> range = {0., 0.};

  /// returns true if value is within range
  /// @param entry the entry in the tree
  bool operator()(ULong64_t entry) {
    if (value != nullptr) {
      float v = value->at(entry);
      return (range[0] <= v && range[1] > v);
    }
    return false;
  }
};

/// This is a direct type accessor
///
/// It simply forwards access to the underlying vector
///
template <typename primitive_t>
struct DirectAccessor {
  std::vector<primitive_t>* value = nullptr;

  /// Gives direct access to the underlying parameter
  ///
  /// @param entry the entry in the tree
  primitive_t operator()(ULong64_t entry) {
    if (value) {
      primitive_t v = value->at(entry);
      return v;
    }
    return std::numeric_limits<primitive_t>::max();
  }
};

// Division accessor
template <typename primitive_one_t, typename primitive_two_t>
struct DivisionAccessor {
  std::vector<primitive_one_t>* one = nullptr;

  std::vector<primitive_two_t>* two = nullptr;

  /// Gives direct access to the underlying parameter
  ///
  /// @param entry the entry in the tree
  primitive_one_t operator()(ULong64_t entry) {
    if (one && two) {
      primitive_one_t vo = one->at(entry);
      primitive_two_t vt = two->at(entry);
      return vo / vt;
    }
    return std::numeric_limits<primitive_one_t>::max();
  }
};

// This is a residual type accessor
struct ResidualAccessor {
  std::vector<float>* value = nullptr;

  std::vector<float>* reference = nullptr;

  /// @return the calculated Residual
  ///
  /// @param entry the entry in the tree
  float operator()(ULong64_t entry) {
    if (value != nullptr && reference != nullptr) {
      float v = value->at(entry);
      float r = reference->at(entry);
      return (v - r);
    }
    return std::numeric_limits<float>::infinity();
  }
};

// This is a  dedicated qop residual accessor
struct QopResidualAccessor {
  std::vector<float>* qop_value = nullptr;

  std::vector<int>* reference_charge = nullptr;

  std::vector<float>* reference_p = nullptr;

  /// @return the calculated Residual for q/p
  ///
  /// @param entry the entry in the tree
  float operator()(ULong64_t entry) {
    if (qop_value != nullptr && reference_charge != nullptr &&
        reference_p != nullptr) {
      float v = qop_value->at(entry);
      float q_true = reference_charge->at(entry);
      float p_true = reference_p->at(entry);
      return (v - q_true / p_true);
    }
    return std::numeric_limits<float>::infinity();
  }
};

/// This the dedicted pT residual accessor
struct PtResidualAccessor {
  std::vector<float>* qop_value = nullptr;

  std::vector<float>* theta_value = nullptr;

  std::vector<float>* reference_pt = nullptr;

  /// @return the calculated Residual
  ///
  /// @param entry the entry in the tree
  float operator()(ULong64_t entry) {
    if (qop_value != nullptr && theta_value != nullptr &&
        reference_pt != nullptr) {
      float p = 1. / std::abs(qop_value->at(entry));
      float theta = theta_value->at(entry);
      float pt_true = reference_pt->at(entry);
      return (p * sin(theta) - pt_true);
    }
    return std::numeric_limits<float>::infinity();
  }
};

// This is a dedicated pT error accessor
struct PtErrorAccessor {
  std::vector<float>* qop_value = nullptr;
  std::vector<float>* qop_error = nullptr;

  std::vector<float>* theta_value = nullptr;
  std::vector<float>* theta_error = nullptr;

  /// @return the calculated error on pT
  ///
  /// @param entry the entry in the tree
  float operator()(ULong64_t entry) {
    if (qop_value != nullptr && qop_error != nullptr &&
        theta_value != nullptr && theta_error != nullptr) {
      float qop_v = qop_value->at(entry);
      float qop_e = qop_error->at(entry);
      float theta_v = theta_value->at(entry);
      float theta_e = theta_error->at(entry);
      return std::cos(theta_v) / qop_v * theta_e -
             std::sin(theta_v) / (qop_v * qop_v) * qop_e;
    }
    return std::numeric_limits<float>::infinity();
  }
};

/// Range estimation for residuals
///
/// @tparam dir_t the type of the directory to change into for writing
/// @tparam tree_t the type of the tree to Draw from
///
/// @param handle the residual/pull handle to be processed
/// @param directory the writable directory
/// @param tree the tree from which is drawn
/// @param peakEntries the number of entries for the range peak
/// @param hBarcode a temporary unique ROOT barcode for memory managements
template <typename dir_t, typename tree_t>
void estimateResiudalRange(ResidualPullHandle& handle, dir_t& directory,
                           tree_t& tree, unsigned long peakEntries,
                           unsigned int hBarcode) {
  // Change into the Directory
  directory.cd();
  TString rangeHist = handle.rangeDrawStr;
  rangeHist += ">>";
  // Hist name snipped
  TString rangeHN = "hrg_";
  rangeHN += hBarcode;
  // Full histogram
  rangeHist += rangeHN;
  rangeHist += handle.rangeMaxStr;

  // Do the drawing
  tree.Draw(rangeHist.Data(), handle.rangeCutStr.c_str(), "", peakEntries);
  handle.rangeHist = dynamic_cast<TH1F*>(gDirectory->Get(rangeHN.Data()));
  if (handle.rangeHist != nullptr) {
    float rms = handle.rangeHist->GetRMS();
    handle.range = {-rms, rms};
  }
}

/// Range estimation for integer values
///
/// @tparam dir_t the type of the directory to change into for writing
/// @tparam tree_t the type of the tree to Draw from
///
/// @param handle the residual/pull handle to be processed
/// @param directory the writable directory
/// @param tree the tree from which is drawn
/// @param peakEntries the number of entries for the range peak
/// @param hBarcode a temporary unique ROOT barcode for memory managements
template <typename dir_t, typename tree_t>
void estimateIntegerRange(SingleHandle& handle, dir_t& directory, tree_t& tree,
                          unsigned long peakEntries, unsigned int startBins,
                          unsigned int addBins, unsigned int hBarcode) {
  // Change into the Directory
  directory.cd();
  TString rangeHist = handle.rangeDrawStr;
  rangeHist += ">>";
  // Hist name snipped
  TString rangeHN = "hrg_";
  rangeHN += hBarcode;
  // Full histogram
  rangeHist += rangeHN;
  rangeHist += "(";
  rangeHist += startBins;
  rangeHist += ",-0.5,";
  rangeHist += static_cast<float>(startBins - 0.5);
  rangeHist += ")";

  unsigned int nBins = startBins;
  // Do the drawing
  tree.Draw(rangeHist.Data(), "", "", peakEntries);
  auto rhist = dynamic_cast<TH1F*>(gDirectory->Get(rangeHN.Data()));
  if (rhist != nullptr) {
    for (unsigned int ib = 1; ib <= startBins; ++ib) {
      if (rhist->GetBinContent(ib) > 0.) {
        nBins = ib;
      }
    }
    handle.bins = (nBins + addBins);
    handle.range = {-0.5, static_cast<float>(handle.bins - 0.5)};
    return;
  }
  handle.bins = (startBins);
  handle.range = {-0.5, static_cast<float>(handle.bins - 0.5)};
}

/// Helper method to book residual and pull histograms
///
/// @param handle the residual/pull handle
/// @param pullRange the symmetric pull range for plotting
/// @param hBins the number of histograms bins
/// @param hBarcoode a temporary unique barcode for ROOT memory management
void bookHistograms(ResidualPullHandle& handle, float pullRange,
                    unsigned int hBins, unsigned int hBarcode) {
  // Residual histogram
  TString rName = std::string("res_") + handle.tag;
  rName += hBarcode;
  handle.residualHist =
      new TH1F(rName.Data(), handle.tag.c_str(), hBins,
               pullRange * handle.range[0], pullRange * handle.range[1]);
  std::string xAxisTitle =
      handle.residualStr + std::string(" ") + handle.residualUnit;
  handle.residualHist->GetXaxis()->SetTitle(xAxisTitle.c_str());
  handle.residualHist->GetYaxis()->SetTitle("Entries");

  // Pull histogram
  TString pName = std::string("pull_") + handle.tag;
  pName += hBarcode;
  handle.pullHist =
      new TH1F(pName.Data(), (std::string("pull ") + handle.tag).c_str(), hBins,
               -pullRange, pullRange);
  xAxisTitle = std::string("(") + handle.residualStr + std::string(")/") +
               handle.errorStr;
  handle.pullHist->GetXaxis()->SetTitle(xAxisTitle.c_str());
  handle.pullHist->GetYaxis()->SetTitle("Entries");
}

/// Helper method to get and opentially overwrite the entries to be processed
///
/// @tparam tree_t the type of the tree
///
/// @param tree is the TTree/TChain in question
/// @param configuredEntries is a configuration parameter
///
/// @return the number of entries
template <typename tree_t>
unsigned long estimateEntries(const tree_t& tree,
                              unsigned long configuredEntries) {
  unsigned long entries = static_cast<unsigned long>(tree.GetEntries());
  if (configuredEntries > 0 && configuredEntries < entries) {
    entries = configuredEntries;
  }
  return entries;
}
