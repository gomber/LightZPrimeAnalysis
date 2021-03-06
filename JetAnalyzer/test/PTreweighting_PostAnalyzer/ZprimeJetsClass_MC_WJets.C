//For use with Ntuples made from JetAnalyzer
////Required arguments: 1 is folder containing input files, 2 is output file path, 3 is maxEvents (-1 to run over all events), 4 is reportEvery
////
////To compile using rootcom to an executable named 'analyze':
////$ ./rootcom ZprimeJetsClass_MC_WJets analyze
////
////To run, assuming this is compiled to an executable named 'analyze':
////$ ./analyze /hdfs/store/user/uhussain/Zprime_Ntuples/ /cms/uhussain/MonoZprimeJet/CMSSW_8_0_8/src/LightZPrimeAnalysis/JetAnalyzer/test/output.root -1 10000
////Runs over every event in the folder Zprime_Ntuples, reporting progress every 10000 events
////and storing the resulting histograms in the file output.root.
////
//
#define ZprimeJetsClass_MC_WJets_cxx
#include "ZprimeJetsClass_MC_WJets.h"
#include <TH2.h>
#include<TH1.h>
#include <TStyle.h>
#include <TCanvas.h>
#include "TH1F.h"
#include <iostream>
#include <bitset>
#include <climits>
#include <cstring>
#include "Math/Minimizer.h"
#include "Math/Factory.h"
#include "Math/Functor.h"
#include "TStopwatch.h"
#include <algorithm>
#include <vector>
#include <iterator>
#include <list>
#include <set>
using namespace std;
using std::vector;
int main(int argc, const char* argv[])
{ 
  Long64_t maxEvents = atof(argv[3]);
  if (maxEvents < -1LL)
  {
    std::cout<<"Please enter a valid value for maxEvents (parameter 3)."<<std::endl;
    return 1;
  }
  int reportEvery = atof(argv[4]);
  if (reportEvery < 1)
  {
    std::cout<<"Please enter a valid value for reportEvery (parameter 4)."<<std::endl;
    return 1;
  }
  //const char* file2 = argv[2];

  ZprimeJetsClass_MC_WJets t(argv[1],argv[2]);
  t.Loop(maxEvents,reportEvery);
  return 0;
}

void ZprimeJetsClass_MC_WJets::Loop(Long64_t maxEvents, int reportEvery)
{
  if (fChain == 0) return;
  int nTotal;
  nTotal = 0;   
   // Book histograms for recording properties of leading jet that passes dR and MET cut
  // TFile* histFile = new TFile(file2, "RECREATE");
  // h_deltar = new TH1F("j1deltaR","j1deltaR; #DeltaR of Leading Jet",50,0,0.51);h_deltar->Sumw2();
  Long64_t nentries = fChain->GetEntries();
  std::cout<<"Coming in: "<<std::endl;
  std::cout<<"nentries:"<<nentries<<std::endl;
  Long64_t nentriesToCheck = nentries;   

  //jetCandidate that passes the basic pt,eta, NHF, CHF cuts
  std::vector<int> jetCand;
  jetCand.clear();

  std::vector<int> jetveto;
  jetveto.clear();

  //jetCandidate that passes dPhiJetMET cut out of the above jetCand
  std::vector<int> jetCand1;
  jetCand1.clear();
 
  TFile *file = new TFile("kfactors.root");
  TH1D *ewkCorrection = (TH1D*)file->Get("EWKcorr/W");
  TH1D *NNLOCorrection = (TH1D*)file->Get("WJets_LO/inv_pt");

  float dphimin=-99;
  //Event is rejected if it contains a HighPtMuon faking MET
  bool muonVeto;

  if (maxEvents != -1LL && nentries > maxEvents)
    nentriesToCheck = maxEvents;
  nTotal = nentriesToCheck;
  Long64_t nbytes = 0, nb = 0;
  std::cout<<"Running over "<<nTotal<<" events."<<std::endl;
  for (Long64_t jentry=0; jentry<nentriesToCheck;jentry++) {
    event_.clear();
    event_info.clear();
    
    Long64_t ientry = LoadTree(jentry);
    if (ientry < 0) break;
    nb = fChain->GetEntry(jentry);   nbytes += nb;

    double event_weight=1.0;
    double EWK_corrected_weight=1.0;
    double NNLO_weight = 1.0;
    double kfactor = 1.0;
    
    muonVeto = HighPtMuonVeto(50.0); //muonPt cut of 50 GeV
    jetCand = getJetCand(200,2.4,0.8,0.1);
    float jet1pT = -99;
    float jet1eta = -99;

    double deltar = 0.0 ;
    deltar= dR(j1etaWidth,j1phiWidth);
    double trk12ptratio = 0.0;
    trk12ptratio = (j1trk12PT/j1PT);
    
    fillHistos(0,kfactor);
    float metcut= 0.0;
    metcut = (fabs(caloMET-pfMET))/pfMET;
    //std::cout<<"|caloMET-pfMET|/pfMET: "<<metcut<<std::endl;
    if (true)
      {
	fillHistos(1,kfactor); 
    	if (metFilters==0) 
      	  {
	    fillHistos(2,kfactor);
	    if (jetCand.size()>0)
	       {
		 fillHistos(3,kfactor);
                 jetveto = JetVetoDecision();
		 if(pfMET>200)
		   {
		    fillHistos(4,kfactor);
		    //std::cout<<"Before electron veto:jetveto.size(): "<<jetveto.size()<<std::endl;
                    if(metcut<0.5)
                      {
                        fillHistos(5,kfactor);
                        double minDPhiJetMET = TMath::Pi();
                        double minDPhiJetMET_first4 = TMath::Pi();
		        for(int j = 0; j < jetveto.size(); j++)
			   {
			    if(DeltaPhi(jetPhi->at(jetveto[j]),pfMETPhi) < minDPhiJetMET)
			       {
			      	minDPhiJetMET = DeltaPhi(jetPhi->at(jetveto[j]),pfMETPhi);
			      	if(j < 4)
				  minDPhiJetMET_first4 = DeltaPhi(jetPhi->at(jetveto[j]),pfMETPhi);
			       }
		      	   }
                        h_dphimin->Fill(minDPhiJetMET_first4);
                        if(dPhiJetMETcut(jetveto))
                          {
                            fillHistos(6,kfactor);
                            if(electron_veto_looseID(jetCand[0],10) &&  muon_veto_looseID(jetCand[0],10))
                              {
																EWK_corrected_weight = event_weight*(ewkCorrection->GetBinContent(ewkCorrection->GetXaxis()->FindBin(bosonPt)));
  															NNLO_weight = event_weight*(NNLOCorrection->GetBinContent(NNLOCorrection->GetXaxis()->FindBin(bosonPt)));
  															if(EWK_corrected_weight!=0 && NNLO_weight!=0){
																kfactor = (EWK_corrected_weight/NNLO_weight);}
																else{kfactor=1.21;}
																std::cout<<"kfactor:"<<kfactor<<std::endl;
                                fillHistos(7,kfactor);
                                h_deltar->Fill(deltar);
				pfMET_prof->Fill(deltar,pfMET,1);
				if(trk12ptratio<=1.0){
                                h_trk12ptratio->Fill(trk12ptratio);}
  				jet1eta = (*jetEta)[jetCand[0]];
				h_jet1eta->Fill(jet1eta);
			        //std::cout<<"events before etawidth cut"<<std::endl;
				//std::cout<<run<<":"<<lumis<<":"<<event<<std::endl;
                                if(btagVeto())
                                  {
																		std::cout<<"kfactor after btagVeto: "<<kfactor<<std::endl;
                                    fillHistos(8,kfactor);
                                    if (j1etaWidth<0.04)
                                      {
                                        h_deltar_cut->Fill(deltar);
                                        pfMET_prof_after->Fill(deltar,pfMET,1);
                                        h_trk12ptratio_afterdR->Fill(trk12ptratio);
                                        fillHistos(9,kfactor);
                                        jet1pT = (*jetPt)[jetCand[0]];
                                        h_jet1pt->Fill(jet1pT);}
                                    if (j1nCarrying90 <=4){
                                        fillHistos(10,kfactor);}
                                        h_j1nCons_1->Fill(j1nCons);
                                    if (j1nCarrying96<=4){
                                        fillHistos(11,kfactor);
                                        }
                                    if (j1nCons<=20){
                                        fillHistos(12,kfactor);
                                          }
                                       }
                                   }
                          }
                     }
                  }
              }
          }
      }
    tree->Fill();

    if (jentry%reportEvery == 0)
       {
       std::cout<<"Finished entry "<<jentry<<"/"<<(nentriesToCheck-1)<<std::endl;
			 //std::cout<<"kfactor:"<<kfactor<<std::endl;
       }
  
  }
   
   
   //save the histograms
//   histFile->Write();
//   histFile->Close();
   
}//Closing the Loop function

void ZprimeJetsClass_MC_WJets::BookHistos(const char* file2)
{
  fileName = new TFile(file2, "RECREATE");
  tree = new TTree("ZprimeJet","ZprimeJet");
  tree->Branch("event_","std::vector<unsigned int>",&event_);
  tree->Branch("event_info","std::vector<double>",&event_info);
  fileName->cd();

  h_jet1pt = new TH1F("h_jet1pt","h_jet1pt; Leading Jet pT",50,100,1000);h_jet1pt->Sumw2();  
  h_jet1eta = new TH1F("h_jet1eta","h_jet1eta; Leading Jet #eta",50,-3.0,3.0);h_jet1eta->Sumw2();
  h_dphimin = new TH1F("h_dphimin","h_dphimin; Minimum dPhiJetMET",50,0,3.2);h_dphimin->Sumw2();
  h_deltar  = new TH1F("h_deltar","h_deltar; #DeltaR for Leading Jet", 50,0,0.5);h_deltar->Sumw2();
  h_j1nCons_1 = new TH1F("h_j1nCons_1","h_j1nCons_1; #Number of Constituents of Leading Jet", 10,0,20);h_j1nCons_1->Sumw2();
  h_deltar_cut = new TH1F("h_deltar_cut","h_deltar_cut; #DeltaR for Leading Pencil Jet",25,0,0.1);h_deltar_cut->Sumw2();
  pfMET_prof = new TProfile("pfMET_prof","pfMET_prof;#DeltaR for Leading Pencil Jet",50,0,0.5);pfMET_prof->Sumw2();
  pfMET_prof_after = new TProfile("pfMET_prof_after","pfMET_prof_after;#DeltaR for Leading Pencil Jet",25,0,0.1);pfMET_prof_after->Sumw2();
  h_trk12ptratio = new TH1F("h_trk12ptratio","h_trk12ptratio;j1Trk12pT/j1pT",25,0,5.0);h_trk12ptratio->Sumw2();
  h_trk12ptratio_afterdR = new TH1F("h_trk12ptratio_afterdR","h_trk12ptratio_afterdR;j1Trk12pT/j1pT",25,0,1.5);h_trk12ptratio_afterdR->Sumw2();  
  for(int i=0; i<13; i++){

     char ptbins[100];
     sprintf(ptbins, "_%d", i);
     std::string histname(ptbins);
     h_HT[i] = new TH1F(("HT"+histname).c_str(), "HT",100,0,2500);h_HT[i]->Sumw2();
     h_nJets[i]   = new TH1F(("nJets"+histname).c_str(), "nJets;Number of Jets", 50, 0, 100);h_nJets[i]->Sumw2();
     h_nGoodJets[i]   = new TH1F(("nGoodJets"+histname).c_str(), "nGoodJets;Number of Jets that pass cut(30 GeV)", 5, 0, 10);h_nGoodJets[i]->Sumw2();
     h_pfMET[i] = new TH1F(("pfMET"+histname).c_str(), "pfMET",50,150,1000);h_pfMET[i] ->Sumw2();
     h_pfMETPhi[i] = new TH1F(("pfMETPhi"+histname).c_str(), "pfMETPhi",50,-4,4);h_pfMETPhi[i]->Sumw2();
     h_j1Pt[i]  = new TH1F(("j1pT"+histname).c_str(), "j1pT;p_{T} of Leading Jet [GeV]", 50, 100, 1000);h_j1Pt[i]->Sumw2();
     h_j1Eta[i] = new TH1F(("j1Eta"+histname).c_str(), "j1Eta; #eta of Leading Jet", 50, -2.5, 2.5);h_j1Eta[i]->Sumw2();
     h_j1Phi[i] = new TH1F(("j1Phi"+histname).c_str(), "j1Phi; #phi of Leading Jet", 50, -3.0, 3.0);h_j1Phi[i]->Sumw2();     
     h_j1nCarrying90[i] = new TH1F(("j1nCarrying90"+histname).c_str(),"j1nCarrying90; No. of Constituents of Leading jet carrying 90% of jet energy",20,0,40);h_j1nCarrying90[i]->Sumw2();
     h_j1nCarrying92[i] = new TH1F(("j1nCarrying92"+histname).c_str(),"j1nCarrying92; No. of Constituents of Leading jet carrying 92% of jet energy",20,0,40);h_j1nCarrying92[i]->Sumw2();
     h_j1nCarrying94[i] = new TH1F(("j1nCarrying94"+histname).c_str(),"j1nCarrying94; No. of Constituents of Leading jet carrying 94% of jet energy",20,0,40);h_j1nCarrying94[i]->Sumw2();
     h_j1nCarrying96[i] = new TH1F(("j1nCarrying96"+histname).c_str(),"j1nCarrying96; No. of Constituents of Leading jet carrying 96% of jet energy",20,0,40);h_j1nCarrying96[i]->Sumw2();
     h_j1nCarrying98[i] = new TH1F(("j1nCarrying98"+histname).c_str(),"j1nCarrying98; No. of Constituents of Leading jet carrying 98% of jet energy",20,0,40);h_j1nCarrying98[i]->Sumw2();     
     h_j1etaWidth[i] = new TH1F(("j1etaWidth"+histname).c_str(),"j1etaWidh; #eta width of Leading Jet", 50,0,0.25);h_j1etaWidth[i] ->Sumw2();
     h_j1phiWidth[i] = new TH1F(("j1phiWidth"+histname).c_str(),"j1phiWidth; #phi width of Leading Jet", 50, 0,0.5);h_j1phiWidth[i]->Sumw2();
     h_j1etaWidthInECal[i] = new TH1F(("j1etaWidthInECal"+histname).c_str(),"j1etaWidthInECal; #eta width in ECAL of Leading Jet", 50,0,0.25); h_j1etaWidthInECal[i]->Sumw2();
     h_j1etaWidthInHCal[i] = new TH1F(("j1etaWidthInHCal"+histname).c_str(),"j1etaWidthInHCal; #eta width in HCAL of Leading Jet", 50,0,0.25);h_j1etaWidthInHCal[i]->Sumw2();
     h_j1phiWidthInECal[i] = new TH1F(("j1phiWidthInECal"+histname).c_str(),"j1phiWidthInECal; #phi width in ECAL of Leading Jet", 50,0,0.5);h_j1phiWidthInECal[i]->Sumw2(); 
     h_j1phiWidthInHCal[i] = new TH1F(("j1phiWidthInHCal"+histname).c_str(),"j1phiWidthInHCal; #phi width in HCAL of Leading Jet", 50,0,0.5);h_j1phiWidthInHCal[i]->Sumw2(); 
     h_j1nCons[i] = new TH1F (("j1nCons"+histname).c_str(),"j1NConstituents; Number of Constituents of Leading Jet",25, 0, 50);h_j1nCons[i]->Sumw2();
     h_j1CEF[i] = new TH1F(("j1CEF"+histname).c_str(), "j1CEF; Charged Electromagnetic Energy Fraction of leading Jet",25, 0, 1.2);h_j1CEF[i]->Sumw2();
     h_j1NEF[i] = new TH1F(("j1NEF"+histname).c_str(), "j1NEF; Neutral Electromagnetic Energy Fraction of leading Jet",25, 0, 1.2);h_j1NEF[i]->Sumw2();
     h_j1CHF[i] = new TH1F(("j1CHF"+histname).c_str(), "j1CHF; Charged Hadron Energy Fraction of leading Jet",25, 0, 1.2);h_j1CHF[i]->Sumw2();
     h_j1NHF[i] = new TH1F(("j1NHF"+histname).c_str(), "j1NHF; Neutral Hadron Energy Fraction of leading Jet",25, 0, 1.2);h_j1NHF[i]->Sumw2();     
     h_j1ntracks[i] = new TH1F(("j1nTracks"+histname).c_str(),"j1nTracks: Number of tracks associated with Leading Jet",10,0,20);h_j1ntracks[i]->Sumw2();
     h_j2ntracks[i] = new TH1F(("j2nTracks"+histname).c_str(),"j2nTracks: Number of tracks associated with Second Leading Jet",10,0,20);h_j2ntracks[i]->Sumw2();     
     h_j1trk12pt[i] = new TH1F(("j1trk12pt"+histname).c_str(),"j1trk12pt: Sum of P_{T} of two leading tracks associated with Leading Jet",50,0,1000);h_j1trk12pt[i]->Sumw2();
     h_j1PFCandsPtSum[i]= new TH1F(("j1PFCandsPtSum"+histname).c_str(),"j1PFCandsPtSum: Sum of P_{T} of all PFCands associated with Leading Jet",50,0,1000);h_j1PFCandsPtSum[i]->Sumw2();
     h_j1PFCand12PtSum[i] = new TH1F(("j1PFCand12PtSum"+histname).c_str(),"j1PFCand12PtSum: Sum of P_{T} of two leading PFCands associated with Leading Jet",50,0,1000);h_j1PFCand12PtSum[i]->Sumw2();
     h_j1PFCand12Ratio[i] = new TH1F(("j1PF12ptRatio"+histname).c_str(),"j1PF12ptRatio: j1PF12Pt/j1PFAllPt",25,0,1.0);h_j1PFCand12Ratio[i]->Sumw2();   
  }
}

double ZprimeJetsClass_MC_WJets::dR(double jetetaWidth, double jetphiWidth)
{
  double deltar = sqrt(jetetaWidth*jetetaWidth + jetphiWidth*jetphiWidth);
  return deltar;
}


void ZprimeJetsClass_MC_WJets::fillHistos(int histoNumber,double kfactor)
{
  h_HT[histoNumber]->Fill(HT,kfactor);
  h_nJets[histoNumber]->Fill(nJets,kfactor);
  h_nGoodJets[histoNumber]->Fill(nGoodJets,kfactor);
  h_pfMET[histoNumber]->Fill(pfMET,kfactor);
  h_pfMETPhi[histoNumber]->Fill(pfMETPhi,kfactor);
  //h_dPhiJetMET[histoNumber]->Fill(dPhimin); 
  h_j1Pt[histoNumber]->Fill(j1PT,kfactor);
  h_j1Eta[histoNumber]->Fill(j1Eta,kfactor);
  h_j1Phi[histoNumber]->Fill(j1Phi,kfactor);
  h_j1nCarrying90[histoNumber]->Fill(j1nCarrying90,kfactor);
  h_j1nCarrying92[histoNumber]->Fill(j1nCarrying92,kfactor);
  h_j1nCarrying94[histoNumber]->Fill(j1nCarrying94,kfactor);
  h_j1nCarrying96[histoNumber]->Fill(j1nCarrying96,kfactor);
  h_j1nCarrying98[histoNumber]->Fill(j1nCarrying98,kfactor);  
  h_j1etaWidth[histoNumber]->Fill(j1etaWidth,kfactor);
  h_j1phiWidth[histoNumber]->Fill(j1phiWidth,kfactor);
  h_j1etaWidthInECal[histoNumber]->Fill(j1etaWidthInECal,kfactor);
  h_j1etaWidthInHCal[histoNumber]->Fill(j1etaWidthInHCal,kfactor);
  h_j1phiWidthInECal[histoNumber]->Fill(j1phiWidthInECal,kfactor);
  h_j1phiWidthInHCal[histoNumber]->Fill(j1phiWidthInHCal,kfactor);
  h_j1nCons[histoNumber]->Fill(j1nCons,kfactor);
  h_j1CEF[histoNumber]->Fill(j1CEmFr,kfactor);
  h_j1NEF[histoNumber]->Fill(j1NEmFr,kfactor);
  h_j1CHF[histoNumber]->Fill(j1CHdFr,kfactor);
  h_j1NHF[histoNumber]->Fill(j1NHdFr,kfactor);
  h_j1ntracks[histoNumber]->Fill(j1nTracks,kfactor);
  h_j2ntracks[histoNumber]->Fill(j2nTracks,kfactor);
  h_j1trk12pt[histoNumber]->Fill(j1trk12PT,kfactor);
  h_j1PFCandsPtSum[histoNumber]->Fill(j1PFCandsPtSum,kfactor);
  h_j1PFCand12PtSum[histoNumber]->Fill(j1PFCand12PtSum,kfactor);
  h_j1PFCand12Ratio[histoNumber]->Fill(j1PFCand12Ratio,kfactor);
}
//Function to calculate regular deltaR separate from jet width variable 'dR'
double ZprimeJetsClass_MC_WJets::deltaR(double eta1, double phi1, double eta2, double phi2)
{
  double deltaeta = abs(eta1 - eta2);
  double deltaphi = DeltaPhi(phi1, phi2);
  double deltar = sqrt(deltaeta*deltaeta + deltaphi*deltaphi);
  return deltar;
}

//Gives the (minimum) separation in phi between the specified phi values
////Must return a positive value
float ZprimeJetsClass_MC_WJets::DeltaPhi(float phi1, float phi2)
{
  float pi = TMath::Pi();
  float dphi = fabs(phi1-phi2);
  if(dphi>pi)
    dphi = 2.0*pi - dphi;
  return dphi;
}

float ZprimeJetsClass_MC_WJets::dPhiJetMETmin(std::vector<int> jets)
{
  float dPhimin=TMath::Pi();
  int njetsMax = jets.size();
  if(njetsMax > 4)
    njetsMax = 4; 
  for(int j=0;j< njetsMax; j++)
    {
      float dPhi = DeltaPhi((*jetPhi)[j],pfMETPhi);
      //std::cout<<"DeltaPhi: "<<dPhi<<std::endl;
      if(dPhi < dPhimin){
        dPhimin = dPhi;
     }
   }
  return dPhimin;
}
bool ZprimeJetsClass_MC_WJets::HighPtMuonVeto(double muonPtCut)
{
  //pass veto if no muon in the event is found with Pt > muonPtCut
  bool passes = true;
  for (int i=0;i<nMu;i++)
    {
      if((*muPt)[i] > muonPtCut){
        passes = false;  
    } 
   }
  return passes;
}
std::vector<int> ZprimeJetsClass_MC_WJets::getJetCand(double jetPtCut, double jetEtaCut, double jetNHFCut, double jetCHFCut){

  std::vector<int> tmpCand;
  tmpCand.clear();

  for(int p=0;p<nJets;p++)
    {

      bool kinematic = (*jetPt)[p] > jetPtCut && (*jetNHF)[p] < jetNHFCut && (*jetCHF)[p] > jetCHFCut && fabs((*jetEta)[p])<jetEtaCut;

      if((*jetPFLooseId)[p]==1 && kinematic){
        tmpCand.push_back(p);
      }
    }

  return tmpCand;

}

std::vector<int> ZprimeJetsClass_MC_WJets::JetVetoDecision() {

  bool jetVeto=true;
  std::vector<int> jetindex;

  for(int i = 0; i < nJets; i++)
    {
      if(jetPt->at(i) >30.0 && jetPFLooseId->at(i)==1)
        {
          jetindex.push_back(i);
        }


    }

  return jetindex;

}

bool ZprimeJetsClass_MC_WJets::btagVeto() {

  bool btagVeto = true;
  for(int i = 0; i < nJets; i++)
    {
      if(jetPt->at(i) >20.0 && jetEta->at(i) < 2.4 && jetCSV2BJetTags->at(i) > 0.8)
        btagVeto = false;
    }
  return btagVeto;
}

bool ZprimeJetsClass_MC_WJets::dPhiJetMETcut(std::vector<int> jets)
{
  //reject jet if it is found within DeltaPhi(jet,MET) < 0.5                                                                                              \
  
  bool passes = false;

  int njetsMax = jets.size();
  //std::cout<<"njets: "<<njetsMax<<std::endl;
  //Only look at first four jets (because that's what monojet analysis do)
  if(njetsMax > 4)
    njetsMax = 4;
  int j=0;
  for(;j< njetsMax; j++){
    //std::cout<<"DeltaPhi b/w Jet and MET"<<std::endl;
    //std::cout<<"jet "<<j<<":"<<DeltaPhi((*jetPhi)[j],pfMETPhi)<<std::endl;
    if(DeltaPhi((*jetPhi)[j],pfMETPhi) < 0.5)
      break;
  }

  if(j==njetsMax)
    passes = true;

  return passes;

}
bool ZprimeJetsClass_MC_WJets::electron_veto_looseID(int jet_index, float elePtCut)
{
  bool veto_passed = true; //pass veto if no good electron found
  
  bool pass_SigmaIEtaIEtaFull5x5 = false;
  bool pass_dEtaIn = false; 
  bool pass_dPhiIn = false;
  bool pass_HoverE = false;
  bool pass_iso = false;
  bool pass_ooEmooP = false;
  bool pass_d0 = false;
  bool pass_dz = false;
  bool pass_missingHits = false;
  bool pass_convVeto = false;
  //Explicitly stating types to avoid a TMath::Max conversion issue                                                                                               
  Float_t EA = 0.0;
  Float_t zero = 0.0;
  Float_t EAcorrIso = 999.9;
  for(int i = 0; i < nEle; i++)
     {
      pass_SigmaIEtaIEtaFull5x5 = false;
      pass_dEtaIn = false;
      pass_dPhiIn = false;
      pass_HoverE = false;
      pass_iso = false;
      pass_ooEmooP = false;
      pass_d0 = false;
      pass_dz = false;
      pass_missingHits = false;
      pass_convVeto = false;
      //Find EA for corrected relative iso.     
      if(abs(eleSCEta->at(i)) <= 1.0)
        EA = 0.1752;
      else if(1.0 < abs(eleSCEta->at(i)) && abs(eleSCEta->at(i)) <= 1.479)
        EA = 0.1862;
      else if(1.479 < abs(eleSCEta->at(i)) && abs(eleSCEta->at(i)) <= 2.0)
        EA = 0.1411;
      else if(2.0 < abs(eleSCEta->at(i)) && abs(eleSCEta->at(i)) <= 2.2)
        EA = 0.1534;
      else if(2.2 < abs(eleSCEta->at(i)) && abs(eleSCEta->at(i)) <= 2.3)
        EA = 0.1903;
      else if(2.3 < abs(eleSCEta->at(i)) && abs(eleSCEta->at(i)) <= 2.4)
        EA = 0.2243;
      else if(2.4 < abs(eleSCEta->at(i)) && abs(eleSCEta->at(i)) < 2.5)
        EA = 0.2687;
      EAcorrIso = (eleisoChargedHadrons->at(i) + TMath::Max(zero,eleisoNeutralHadrons->at(i) + eleisoPhotons->at(i) - rho*EA))/(elePt->at(i)); 
      if(abs(eleSCEta->at(i)) <= 1.479)
        {
          pass_SigmaIEtaIEtaFull5x5 = eleSigmaIEtaIEtaFull5x5->at(i) < 0.0103;
          pass_dEtaIn = abs(eledEtaAtVtx->at(i)) < 0.0105;
          pass_dPhiIn = abs(eledPhiAtVtx->at(i)) < 0.115;
          pass_HoverE = eleHoverE->at(i) < 0.104;
          pass_iso = EAcorrIso < 0.0893;
          pass_ooEmooP = eleEoverPInv->at(i) < 0.102;
          pass_d0 = abs(eleD0->at(i)) < 0.0261;
          pass_dz = abs(eleDz->at(i)) < 0.41;
          pass_missingHits = eleMissHits->at(i) <= 2;
          pass_convVeto = eleConvVeto->at(i) == 1;
        }
      else if(1.479 < abs(eleSCEta->at(i)) < 2.5)
        {
          pass_SigmaIEtaIEtaFull5x5 = eleSigmaIEtaIEtaFull5x5->at(i) < 0.0301;
          pass_dEtaIn = abs(eledEtaAtVtx->at(i)) < 0.00814;
          pass_dPhiIn = abs(eledPhiAtVtx->at(i)) < 0.182;
          pass_HoverE = eleHoverE->at(i) < 0.0897;
          pass_iso = EAcorrIso < 0.121;
          pass_ooEmooP = eleEoverPInv->at(i) < 0.126;
          pass_d0 = abs(eleD0->at(i)) < 0.118;
          pass_dz = abs(eleDz->at(i)) < 0.822;
          pass_missingHits = eleMissHits->at(i) <= 1;
          pass_convVeto = eleConvVeto->at(i) == 1;
        } 
      //Electron passes Loose Electron ID cuts  
      if(pass_SigmaIEtaIEtaFull5x5 && pass_dEtaIn && pass_dPhiIn && pass_HoverE && pass_iso && pass_ooEmooP && pass_d0 && pass_dz && pass_missingHits && pass_convVeto)
        { 
          //Electron passes pt cut                                                                                                                                
          if(elePt->at(i) > elePtCut)
            {
              //Electron does not overlap with jet
              if(deltaR(eleSCEta->at(i),eleSCPhi->at(i),jetEta->at(jet_index),jetPhi->at(jet_index)) > 0.5)
                {
                  veto_passed = false;
                  break;
                }
            }   
        }         
    }             
  return veto_passed;
}                  
bool ZprimeJetsClass_MC_WJets::muon_veto_looseID(int jet_index, float muPtCut)
{
  bool veto_passed = true; //pass veto if no good muon found 
  bool pass_iso = false;  
                                                                                                                                                        
  Float_t zero = 0.0;
  Float_t muPhoPU = 999.9;
  Float_t tightIso_combinedRelative = 999.9;
    
  for(int i = 0; i < nMu; i++)
    {
      muPhoPU = muPFNeuIso->at(i) + muPFPhoIso->at(i) - 0.5*muPFPUIso->at(i);
      tightIso_combinedRelative = (muPFChIso->at(i) + TMath::Max(zero,muPhoPU))/(muPt->at(i));
      pass_iso = tightIso_combinedRelative < 0.25;

      if(muPt->at(i) > muPtCut)
        {
          if(pass_iso)
            {
	      //muon does not overlap with jet
              if(deltaR(muEta->at(i),muPhi->at(i),jetEta->at(jet_index),jetPhi->at(jet_index)) > 0.5)
                {
                  veto_passed = false;
                  break;
                }
            }
        }
     }      
    return veto_passed;
}                
  
