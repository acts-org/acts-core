/*
 * SPDX-PackageName: "ACTS"
 * SPDX-FileCopyrightText: 2016 CERN
 * SPDX-License-Identifier: MPL-2.0
 */

#include <TROOT.h>

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std ;

/// Function used to make ratio plot.

void Draw_ratio(TCanvas* c, TProfile* h1, TProfile* h2, TLegend* leg, std::string name){

  // Upper plot will be in pad1
  TPad *pad1 = new TPad("pad1", "pad1", 0, 0.3, 1, 1.0);

  pad1->SetBottomMargin(0); // Upper and lower plot are joined
  pad1->SetGridx();         // Vertical grid
  pad1->Draw();             // Draw the upper pad: pad1
  pad1->cd();               // pad1 becomes the current pad
  h1->Draw("E");
  h2->Draw("E SAME");
  leg->Draw("SAME");

  double max_hist [4];
  max_hist[0] = h1->GetMaximum()+h1->GetMaximum()*0.05;
  max_hist[1] = h2->GetMaximum()+h2->GetMaximum()*0.05;

  h1->GetYaxis()->SetTitleOffset(1.5);
  h1->GetYaxis()->SetRangeUser(0, *max_element( begin(max_hist),end(max_hist) ) );

  // lower plot will be in pad
  c->cd();          // Go back to the main canvas before defining pad2
  TPad *pad2 = new TPad("pad2", "pad2", 0, 0.05, 1, 0.3);
  pad2->SetTopMargin(0);
  pad2->SetBottomMargin(0.2);
  pad2->SetGridx(); // vertical grid
  pad2->Draw();
  pad2->cd();       // pad2 becomes the current pad

  TLine line = TLine(h1->GetXaxis()->GetXmin(),1,h1->GetXaxis()->GetXmax(),1);
  line.SetLineColor(kRed);
  line.SetLineWidth(1);

  // Define the ratio plot
  TProfile *h5 = (TProfile*)h2->Clone( (name+"_ratio").c_str() );
  h5->SetLineColor(kBlack);

  h5->SetStats(0);      // No statistics on lower plot
  h5->Divide(h1);

  double maxi = min( max( std::abs(h5->GetMinimum()-0.1*h5->GetMinimum()),h5->GetMaximum()+0.1*h5->GetMaximum() ), 10. );

  h5->SetMinimum( 0.5 );  // Define Y ..
  h5->SetMaximum( 1.1 ); // .. range

  h5->SetMarkerStyle(7);
  h5->Draw("hist P");       // Draw the ratio plot
  line.Draw("SAME");

  // Y axis ratio plot settings
  h5->GetYaxis()->SetTitle("Ratio Validation/Geantino ");
  h5->GetYaxis()->SetNdivisions(505);
  h5->GetYaxis()->SetTitleSize(15);
  h5->GetYaxis()->SetTitleFont(43);
  h5->GetYaxis()->SetTitleOffset(1.55);
  h5->GetYaxis()->SetLabelFont(43);
  h5->GetYaxis()->SetLabelSize(15);

  // X axis ratio plot settings
  h5->GetXaxis()->SetTitleSize(20);
  h5->GetXaxis()->SetTitleFont(43);
  h5->GetXaxis()->SetTitleOffset(3);
  h5->GetXaxis()->SetLabelFont(43);
  h5->GetXaxis()->SetLabelSize(15);

  std::string name_axis = h1->GetXaxis()->GetTitle();

  c->Print( (name+"/Ratio_Val_geant_mat_X0_"+name_axis+".pdf").c_str());

  delete h5;

}

/// Compare two set of material tracks (for example one obtain with propagator and material map and one with geantino scan).
/// Draw the ammont of material (in X0) encounter by tracks as function of eta and phi.
/// Plot the ratio between the two set to help identify inconsistency.

void Mat_map(std::string Val = "", std::string geantino = "", std::string name = ""){

  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  TProfile * Val_X0_Eta = new TProfile("Val_X0_Eta","Val_X0_Eta",160,-4,4);
  TProfile * Val_X0_Phi = new TProfile("Val_X0_Phi","Val_X0_Phi",160,-4,4);
  TH2F * Val_X0_Eta_spread = new TH2F("Val_X0_Eta_spread","Val_X0_Eta_spread",160,-4,4,160,0,4);
  TH2F * Val_X0_Phi_spread = new TH2F("Val_X0_Phi_spread","Val_X0_Phi_spread",160,-4,4,160,0,4);
  TH2F * Val_X0 = new TH2F("Val_X0","Val_X0",160,-4,4,160,-4,4);

  TProfile * geantino_X0_Eta = new TProfile("geantino_X0_Eta","geantino_X0_Eta",160,-4,4);
  TProfile * geantino_X0_Phi = new TProfile("geantino_X0_Phi","geantino_X0_Phi",160,-4,4);
  TH2F * geantino_X0_Eta_spread = new TH2F("geantino_X0_Eta_spread","geantino_X0_Eta_spread",160,-4,4,160,0,4);
  TH2F * geantino_X0_Phi_spread = new TH2F("geantino_X0_Phi_spread","geantino_X0_Phi_spread",160,-4,4,160,0,4);
  TH2F * geantino_X0 = new TH2F("geantino_X0","geantinol_X0",160,-4,4,160,-4,4);

  TH1F * comp_X0_Eta = new TH1F("comp_X0_Eta","comp_X0_Eta",160,-4,4);
  TH1F * comp_X0_Phi = new TH1F("comp_X0_Phi","comp_X0_Phi",160,-4,4);
  TH2F * comp_X0 = new TH2F("comp_X0","comp_X0",160,-4,4,160,-4,4);

  TChain *Val_file = new TChain("material-tracks");
  TChain *geantino_file = new TChain("material-tracks");

  // Define line corresponding to the different eta value
  TLine *eta_0 = new TLine(0,-1200,0,1200);
  eta_0->SetLineColor(kRed);

  TLine *eta_1p = new TLine(-1250,-1064,1250,1064);
  eta_1p->SetLineColor(kRed);
  TLine *eta_2p = new TLine(-3000,-827,3000,827);
  eta_2p->SetLineColor(kRed);
  TLine *eta_3p = new TLine(-3000,-300,3000,300);
  eta_3p->SetLineColor(kRed);
  TLine *eta_4p = new TLine(-3000,-110,3000,110);
  eta_4p->SetLineColor(kRed);

  TLine *eta_1n = new TLine(-1250,1064,1250,-1064);
  eta_1n->SetLineColor(kRed);
  TLine *eta_2n = new TLine(-3000,827,3000,-827);
  eta_2n->SetLineColor(kRed);
  TLine *eta_3n = new TLine(-3000,300,3000,-300);
  eta_3n->SetLineColor(kRed);
  TLine *eta_4n = new TLine(-3000,110,3000,-110);
  eta_4n->SetLineColor(kRed);

  if(Val != ""){
    Val_file->Add(Val.c_str());

    // 2D map for Validation input
    TCanvas *VM = new TCanvas("VM","Validation Map") ;
    Val_file->Draw("mat_y:mat_z","std::abs(mat_x)<1");

    eta_0->Draw("Same");
    eta_1p->Draw("Same");
    eta_1n->Draw("Same");
    eta_2p->Draw("Same");
    eta_2n->Draw("Same");
    eta_3p->Draw("Same");
    eta_3n->Draw("Same");
    eta_4p->Draw("Same");
    eta_4n->Draw("Same");

    VM->Print( (name+"/Val_mat_map.png").c_str());
    //VM->Print( (name+"/Val_mat_map.pdf").c_str());

    // X0 as function of Eta for Validation input
    TCanvas *VM_X0_Eta = new TCanvas("VM_X0_Eta","Validation X0 Eta") ;
    Val_file->Draw("t_X0:v_eta>>Val_X0_Eta","","profile");
    Val_X0_Eta->SetMarkerStyle(7);
    Val_X0_Eta->Draw("HIST PC");
    Val_X0_Eta->GetXaxis()->SetTitle("Eta");
    Val_X0_Eta->GetYaxis()->SetTitle("X0");
    VM_X0_Eta->Print( (name+"/Val_mat_Eta_X0.pdf").c_str());

    // X0 as function of Eta for Validation input
    TCanvas *VM_X0_Eta_spread = new TCanvas("VM_X0_Eta_spread","Validation X0 Eta") ;
    Val_file->Draw("t_X0:v_eta>>Val_X0_Eta_spread","","");
    Val_X0_Eta_spread->GetXaxis()->SetTitle("Eta");
    Val_X0_Eta_spread->GetYaxis()->SetTitle("X0");
    VM_X0_Eta_spread->Print( (name+"/Val_X0_Eta_spread.pdf").c_str());

    // X0 as function of Phi for Validation input
    TCanvas *VM_X0_Phi = new TCanvas("VM_X0_Phi","Validation X0 Phi") ;
    Val_file->Draw("t_X0:v_phi>>Val_X0_Phi","","profile");
    Val_X0_Phi->SetMarkerStyle(7);
    Val_X0_Phi->Draw("HIST PC");
    Val_X0_Phi->GetXaxis()->SetTitle("Phi");
    Val_X0_Phi->GetYaxis()->SetTitle("X0");
    VM_X0_Phi->Print( (name+"/Val_mat_Phi_X0.pdf").c_str());

    // X0 as function of Phi for Validation input
    TCanvas *VM_X0_Phi_spread = new TCanvas("VM_X0_Phi_spread","Validation X0 Phi") ;
    Val_file->Draw("t_X0:v_phi>>Val_X0_Phi_spread","","");
    Val_X0_Phi_spread->GetXaxis()->SetTitle("Phi");
    Val_X0_Phi_spread->GetYaxis()->SetTitle("X0");
    VM_X0_Phi_spread->Print( (name+"/Val_mat_Phi_X0_spread.pdf").c_str());

    // 2D map of X0 for Validation input
    TCanvas *VM_2D = new TCanvas("VM_2D","Validation X0 2D") ;
    Val_file->Draw("v_phi:v_eta:t_X0>>Val_X0","","COLZ");
    Val_X0->GetXaxis()->SetTitle("Eta");
    Val_X0->GetYaxis()->SetTitle("Phi");
    Val_X0->GetZaxis()->SetTitle("X0");
    VM_2D->Print( (name+"/Val_mat_X0.png").c_str());
  }

  if(geantino != ""){
    geantino_file->Add(geantino.c_str());

    // 2D map for Geantino input
    TCanvas *GM = new TCanvas("GM","Geantino Map") ;
    geantino_file->Draw("mat_y:mat_z","std::abs(mat_x)<1");

    eta_0->Draw("Same");
    eta_1p->Draw("Same");
    eta_1n->Draw("Same");
    eta_2p->Draw("Same");
    eta_2n->Draw("Same");
    eta_3p->Draw("Same");
    eta_3n->Draw("Same");
    eta_4p->Draw("Same");
    eta_4n->Draw("Same");

    GM->Print( (name+"/geant_mat_map.png").c_str());
    //GM->Print( (name+"/geant_mat_map.pdf").c_str());

    // X0 as function of Eta for Geantino input
    TCanvas *GM_X0_Eta = new TCanvas("GM_X0_Eta","Geantino X0 Eta") ;
    geantino_file->Draw("t_X0:v_eta>>geantino_X0_Eta","","profile");
    geantino_X0_Eta->SetMarkerStyle(7);
    geantino_X0_Eta->Draw("HIST PC");
    geantino_X0_Eta->GetXaxis()->SetTitle("Eta");
    geantino_X0_Eta->GetYaxis()->SetTitle("X0");
    GM_X0_Eta->Print( (name+"/geant_mat_Eta_X0.pdf").c_str());

    // X0 as function of Eta for Geantino input
    TCanvas *GM_X0_Eta_spread = new TCanvas("GM_X0_Eta_spread","Geantino X0 Eta") ;
    geantino_file->Draw("t_X0:v_eta>>geantino_X0_Eta_spread","","");
    geantino_X0_Eta_spread->GetXaxis()->SetTitle("Eta");
    geantino_X0_Eta_spread->GetYaxis()->SetTitle("X0");
    GM_X0_Eta_spread->Print( (name+"/geant_mat_Eta_X0_spread.pdf").c_str());

    // X0 as function of Phi for Geantino input
    TCanvas *GM_X0_Phi = new TCanvas("GM_X0_Phi","Geantino X0 Phi") ;
    geantino_file->Draw("t_X0:v_phi>>geantino_X0_Phi","","profile");
    geantino_X0_Phi->SetMarkerStyle(7);
    geantino_X0_Phi->Draw("HIST PC");
    geantino_X0_Phi->GetXaxis()->SetTitle("Phi");
    geantino_X0_Phi->GetYaxis()->SetTitle("X0");
    GM_X0_Phi->Print( (name+"/geant_mat_Phi_X0.pdf").c_str());

    // X0 as function of Phi for Geantino input
    TCanvas *GM_X0_Phi_spread = new TCanvas("GM_X0_Phi_spread","Geantino X0 Phi") ;
    geantino_file->Draw("t_X0:v_phi>>geantino_X0_Phi_spread","","");
    geantino_X0_Phi_spread->GetXaxis()->SetTitle("Phi");
    geantino_X0_Phi_spread->GetYaxis()->SetTitle("X0");
    GM_X0_Phi_spread->Print( (name+"/geant_mat_Phi_X0_spread.pdf").c_str());

    // 2D map of X0 for Geantino input
    TCanvas *GM_2D = new TCanvas("GM_2D","Geantino X0 2D") ;
    geantino_file->Draw("v_phi:v_eta:t_X0>>geantino_X0","","COLZ");
    geantino_X0->GetXaxis()->SetTitle("Eta");
    geantino_X0->GetYaxis()->SetTitle("Phi");
    geantino_X0->GetZaxis()->SetTitle("X0");
    GM_2D->Print( (name+"/geant_mat_X0.png").c_str());
  }

  if(Val != "" && geantino != ""){

    Val_X0_Eta->SetMarkerColor(kBlack);
    Val_X0_Eta->SetLineColor(kBlack);
    geantino_X0_Eta->SetMarkerColor(kRed);
    geantino_X0_Eta->SetLineColor(kRed);

    Val_X0_Phi->SetMarkerColor(kBlack);
    Val_X0_Phi->SetLineColor(kBlack);
    geantino_X0_Phi->SetMarkerColor(kRed);
    geantino_X0_Phi->SetLineColor(kRed);

    TLegend* leg = new TLegend(0.1,0.15,0.25,0.30);
    leg->AddEntry(Val_X0_Eta,"Validation X0");
    leg->AddEntry(geantino_X0_Eta,"Geantino X0");

    TLegend* leg2 = new TLegend(0.1,0.15,0.25,0.30);
    leg2->AddEntry(Val_X0_Phi,"Validation X0");
    leg2->AddEntry(geantino_X0_Phi,"Geantino X0");

    // X0 differences as function of eta of the Validation and Geantino input
    comp_X0_Eta->Add(Val_X0_Eta);
    comp_X0_Eta->Add(geantino_X0_Eta,-1);
    comp_X0_Eta->Divide(Val_X0_Eta);
    comp_X0_Eta->Scale(100);
    comp_X0_Eta->GetXaxis()->SetTitle("Eta");
    comp_X0_Eta->GetYaxis()->SetTitle("(Validation X0 - geantino X0) / Validation X0 [%]");
    TCanvas *Diff_Eta = new TCanvas("Diff_Eta","Comparison geantino_Validation Eta") ;
    comp_X0_Eta->Draw();
    Diff_Eta->Print( (name+"/Comp_Val_geant_mat_X0_Eta.pdf").c_str());

    // X0 comparison as function of eta of the Validation and Geantino input
    TCanvas *Comp_Eta_spread = new TCanvas("Comp_Eta_spread","Comparison geantino_Validation Eta") ;
    Val_X0_Eta_spread->Draw();
    geantino_X0_Eta_spread->SetMarkerColor(kRed);
    geantino_X0_Eta_spread->Draw("SAME");
    leg->Draw("SAME");
    Comp_Eta_spread->Print( (name+"/Comp_Val_geant_mat_X0_Eta_spread.pdf").c_str());

    // X0 differences as function of phi of the Validation and Geantino input
    comp_X0_Phi->Add(Val_X0_Phi);
    comp_X0_Phi->Add(geantino_X0_Phi,-1);
    comp_X0_Phi->Divide(Val_X0_Phi);
    comp_X0_Phi->Scale(100);
    comp_X0_Phi->GetXaxis()->SetTitle("Phi");
    comp_X0_Phi->GetYaxis()->SetTitle("(Validation X0 - geantino X0) / Validation X0 [%]");
    TCanvas *Diff_Phi = new TCanvas("Diff_Phi","Comparison geantino_Validation") ;
    comp_X0_Phi->Draw();
    Diff_Phi->Print( (name+"/Comp_Val_geant_mat_X0_Phi.pdf").c_str());

    // X0 comparison as function of eta of the Validation and Geantino input
    TCanvas *Comp_Phi_spread = new TCanvas("Comp_Phi_spread","Comparison geantino_Validation Phi") ;
    Val_X0_Phi_spread->Draw();
    geantino_X0_Phi_spread->SetMarkerColor(kRed);
    geantino_X0_Phi_spread->Draw("SAME");
    leg2->Draw("SAME");
    Comp_Phi_spread->Print( (name+"/Comp_Val_geant_mat_X0_Phi_spread.pdf").c_str());

    Float_t score = 0;
    for(int i=0; i<Val_X0->GetXaxis()->GetNbins(); i++){
      score += (Val_X0->GetBinContent(i+1)-geantino_X0->GetBinContent(i+1))*
      (Val_X0->GetBinContent(i+1)-geantino_X0->GetBinContent(i+1))/
      geantino_X0->GetBinError(i+1)*geantino_X0->GetBinError(i+1);
    }
    std::cout << "Score : " << score << std::endl;

    TText *t = new TText(0,10, ("Chi^2 score : " + to_string(score)).c_str());
    t->Draw();

    // X0 ratio plot as function of eta of the Validation and Geantino input
    TCanvas *Comp_Eta = new TCanvas("Comp_Eta","Ratio geantino_Validation Eta") ;
    Val_X0_Eta->SetMarkerStyle(7);
    geantino_X0_Eta->SetMarkerStyle(7);
    Val_X0_Eta->SetMarkerColor(kBlack);
    geantino_X0_Eta->SetMarkerColor(kRed);
    Val_X0_Eta->SetLineColor(kBlack);
    geantino_X0_Eta->SetLineColor(kRed);

    Draw_ratio(Comp_Eta, geantino_X0_Eta, Val_X0_Eta, leg, name);

    // X0 ratio plot as function of phi of the Validation and Geantino input
    TCanvas *Comp_Phi = new TCanvas("Comp_Phi","Ratio geantino_Validation Phi") ;
    Val_X0_Phi->SetMarkerStyle(7);
    geantino_X0_Phi->SetMarkerStyle(7);
    Val_X0_Phi->SetMarkerColor(kBlack);
    geantino_X0_Phi->SetMarkerColor(kRed);
    Val_X0_Phi->SetLineColor(kBlack);
    geantino_X0_Phi->SetLineColor(kRed);

    Draw_ratio(Comp_Phi, geantino_X0_Phi, Val_X0_Phi, leg, name);
  }

  return;

}
