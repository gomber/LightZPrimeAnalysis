// -*- C++ -*-
//
// Package:    LightZPrimeAnalysis/JetAnalyzer
// Class:      JetAnalyzer
// 
/**\class JetAnalyzer JetAnalyzer.cc LightZPrimeAnalysis/JetAnalyzer/plugins/JetAnalyzer.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Sridhara Rao Dasu
//         Created:  Tue, 23 Feb 2016 04:57:10 GMT
// Edited by: Usama Hussain
//


// system include files
#include <memory>
#include <vector>
#include <list>
using namespace std;

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "HLTrigger/HLTcore/interface/HLTConfigProvider.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/METReco/interface/PFMET.h"

#include "LightZPrimeAnalysis/JetWidthCalculator/interface/JetWidthCalculator.hh"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "TTree.h"

//
// class declaration
//

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<> and also remove the line from
// constructor "usesResource("TFileService");"
// This will improve performance in multithreaded jobs.

class JetAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources>  {
   public:
      explicit JetAnalyzer(const edm::ParameterSet&);
      ~JetAnalyzer();

      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);


   private:
      virtual void beginJob() override;
      virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
      virtual void endJob() override;

      // ----------member data ---------------------------

  edm::EDGetTokenT< vector<reco::PFCandidate> > pfCandsToken;
  edm::EDGetTokenT< vector<reco::PFJet> > pfJetsToken;
  edm::EDGetTokenT< vector<reco::PFMET> > pfMETsToken;
  edm::EDGetTokenT<bool> globalHandle_;
  edm::EDGetTokenT<bool> hcalNoiseHandle_;
  edm::EDGetTokenT<bool> hcalIsoNoiseHandle_;
  edm::EDGetTokenT<bool> eCALTPHandle_;
  edm::EDGetTokenT<bool> bADSCHandle_;
  //edm::EDGetTokenT<bool> goodvertextHandle_;

  edm::EDGetTokenT<edm::TriggerResults>            trgResultsLabel_;

  vector<float> jetPt_;
  vector<float> jetEn_;
  vector<float> jetEta_;
  vector<float> jetPhi_;
  vector<float> jetCHF_;
  vector<float> jetNHF_;
  vector<float> jetCEF_;
  vector<float> jetNEF_;
  vector<int>   jetNCH_;
  vector<float> jetHFHAE_;
  vector<float> jetHFEME_;
  vector<int>   jetNConstituents_;
  vector<bool>  jetPFLooseId_;  
  uint32_t metFilters_;
  double totalET;
  double HT;
  double METValue;
  double METPhi;
  double j1PT;
  double j1Eta;
  double j1Phi;
  double j1CHdFr;
  double j1NHdFr;
  double j1CEmFr;
  double j1NEmFr;
  double j1etaWidth;
  double j1phiWidth;
  double j1etaWidthInECal;
  double j1phiWidthInECal;
  double j1etaWidthInHCal;
  double j1phiWidthInHCal;
  double j2PT;
  double j2Eta;
  double j2Phi;
  double j2CHdFr;
  double j2NHdFr;
  double j2CEmFr;
  double j2NEmFr;
  double j2etaWidth;
  double j2phiWidth;
  double j2etaWidthInECal;
  double j2phiWidthInECal;
  double j2etaWidthInHCal;
  double j2phiWidthInHCal;

  uint32_t nJets;
  uint32_t nGoodJets;

  uint32_t j1nCons;
  uint32_t j2nCons;
  int HLTMET90_;
  TTree* tree;

};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
JetAnalyzer::JetAnalyzer(const edm::ParameterSet& iConfig)
{

  pfCandsToken = consumes< vector<reco::PFCandidate> >(edm::InputTag("particleFlow"));
  pfJetsToken = consumes< vector<reco::PFJet> >(edm::InputTag("ak4PFJetsCHS"));
  pfMETsToken = consumes< vector<reco::PFMET> >(edm::InputTag("pfMet"));
  trgResultsLabel_ = consumes<edm::TriggerResults>  (edm::InputTag("TriggerResults", "", "HLT"));
  globalHandle_= consumes<bool>(edm::InputTag("globalTightHalo2016Filter", ""));  
  hcalNoiseHandle_ = consumes<bool>(edm::InputTag("HBHENoiseFilterResultProducer", "HBHENoiseFilterResult"));
  hcalIsoNoiseHandle_ = consumes<bool>(edm::InputTag("HBHENoiseFilterResultProducer", "HBHEIsoNoiseFilterResult"));
  eCALTPHandle_ =  consumes<bool>(edm::InputTag("EcalDeadCellTriggerPrimitiveFilter", "")); 
  bADSCHandle_ = consumes<bool>(edm::InputTag("eeBadScFilter", ""));
  //goodvertextHandle_ = consumes<bool>(edm::InputTag("goodVertices","offlinePrimaryVertices"));

  usesResource("TFileService");
  edm::Service<TFileService> fs;
  tree = fs->make<TTree>("JetTree", "Jet data for analysis");

  
  tree->Branch("metFilters", &metFilters_);
  tree->Branch("totalET", &totalET);
  tree->Branch("HT", &HT);
  tree->Branch("METValue", &METValue);
  tree->Branch("METPhi", &METPhi);
  tree->Branch("nJets", &nJets);
  tree->Branch("jetPt",&jetPt_);
  tree->Branch("jetEta",          &jetEta_);
  tree->Branch("jetEn",           &jetEn_);
  tree->Branch("jetPhi",          &jetPhi_);
  tree->Branch("jetPFLooseId", &jetPFLooseId_);
  tree->Branch("jetCHF",       &jetCHF_);
  tree->Branch("jetNHF",       &jetNHF_);
  tree->Branch("jetCEF",       &jetCEF_);
  tree->Branch("jetNEF",       &jetNEF_);
  tree->Branch("jetNCH",       &jetNCH_); 
  tree->Branch("jetHFHAE",         &jetHFHAE_);
  tree->Branch("jetHFEME",         &jetHFEME_);
  tree->Branch("jetNConstituents", &jetNConstituents_);
  tree->Branch("nGoodJets", &nGoodJets);
  tree->Branch("j1PT", &j1PT);
  tree->Branch("j1Eta", &j1Eta);
  tree->Branch("j1Phi", &j1Phi);
  tree->Branch("j1CHdFr", &j1CHdFr);
  tree->Branch("j1NHdFr", &j1NHdFr);
  tree->Branch("j1CEmFr", &j1CEmFr);
  tree->Branch("j1NEmFr", &j1NEmFr);
  tree->Branch("j1nCons", &j1nCons);
  tree->Branch("j1etaWidth", &j1etaWidth);
  tree->Branch("j1phiWidth", &j1phiWidth);
  tree->Branch("j1etaWidthInECal", &j1etaWidthInECal);
  tree->Branch("j1phiWidthInECal", &j1phiWidthInECal);
  tree->Branch("j1etaWidthInHCal", &j1etaWidthInHCal);
  tree->Branch("j1phiWidthInHCal", &j1phiWidthInHCal);
  tree->Branch("j2PT", &j2PT);
  tree->Branch("j2Eta", &j2Eta);
  tree->Branch("j2Phi", &j2Phi);
  tree->Branch("j2CHdFr", &j2CHdFr);
  tree->Branch("j2NHdFr", &j2NHdFr);
  tree->Branch("j2CEmFr", &j2CEmFr);
  tree->Branch("j2NEmFr", &j2NEmFr);
  tree->Branch("j2nCons", &j2nCons);
  tree->Branch("j2etaWidth", &j2etaWidth);
  tree->Branch("j2phiWidth", &j2phiWidth);
  tree->Branch("j2etaWidthInECal", &j2etaWidthInECal);
  tree->Branch("j2phiWidthInECal", &j2phiWidthInECal);
  tree->Branch("j2etaWidthInHCal", &j2etaWidthInHCal);
  tree->Branch("j2phiWidthInHCal", &j2phiWidthInHCal);
  tree->Branch("HLTMET90",               &HLTMET90_);
}


JetAnalyzer::~JetAnalyzer()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called for each event  ------------
void
JetAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
   using namespace edm;
   metFilters_ = 0;
   if (iEvent.isRealData()){
    
     Handle<bool> globalHandle;
     iEvent.getByToken(globalHandle_, globalHandle);
     bool GlobalHaloResult_ = *globalHandle;
    
   //  Handle<bool>goodvertextHandle;
   //  iEvent.getByToken(goodvertextHandle_,goodvertextHandle);
   //  bool goodVertexResult_ = *goodvertextHandle;
 
     Handle<bool> hcalNoiseHandle;
     iEvent.getByToken(hcalNoiseHandle_, hcalNoiseHandle);
     bool HBHENoiseResult_ = *hcalNoiseHandle;
     
     Handle<bool> hcalIsoNoiseHandle;
     iEvent.getByToken(hcalIsoNoiseHandle_, hcalIsoNoiseHandle);
     bool HBHEIsoNoiseResult_ = *hcalIsoNoiseHandle;
     
     Handle<bool> eCALTPHandle;
     iEvent.getByToken(eCALTPHandle_, eCALTPHandle);
     bool EcalDeadCellTFResult_ = *eCALTPHandle;

     Handle<bool> bADSCHandle;
     iEvent.getByToken(bADSCHandle_, bADSCHandle);
     bool EEBadSCResult_ = *bADSCHandle;

   if ( !HBHENoiseResult_      ) metFilters_ += 1; 
   if ( !HBHEIsoNoiseResult_   ) metFilters_ += 2; 
   if ( !GlobalHaloResult_        ) metFilters_ += 4; std::cout <<"globalHaloFilterFired"<<std::endl;
 //  if ( !goodVertexResult_     ) metFilters_ += 8; std::cout <<"goodvertexFired"<<std::endl;
   if ( !EEBadSCResult_        ) metFilters_ += 16; std::cout <<"eescfilterFired"<<std::endl;
   if ( !EcalDeadCellTFResult_ ) metFilters_ += 32; 
   }

   Handle< vector<reco::PFCandidate> > pfCands;
   iEvent.getByToken(pfCandsToken, pfCands);

   Handle< vector<reco::PFJet> > pfJets;
   iEvent.getByToken(pfJetsToken, pfJets);

   Handle< vector<reco::PFMET> > pfMETs;
   iEvent.getByToken(pfMETsToken, pfMETs);

   //HLT treatment
   HLTMET90_               = 0;

   Handle<edm::TriggerResults> trgResultsHandle;
   iEvent.getByToken(trgResultsLabel_, trgResultsHandle);
  
  
   const edm::TriggerNames &trgNames = iEvent.triggerNames(*trgResultsHandle);
   
   for (size_t i = 0; i < trgNames.size(); ++i) {
    const string &name = trgNames.triggerName(i);
    //Jet triggers
  
     if (name.find("HLT_PFMET90_PFMHT90_IDTight") != string::npos) {HLTMET90_ = (trgResultsHandle->accept(i)) ? 1 : 0;}

}
   //Clear previous events
   jetPt_.clear();
   jetEn_.clear();
   jetEta_.clear();
   jetPhi_.clear();
   jetPFLooseId_                           .clear();
   jetCHF_                                 .clear();
   jetNHF_                                 .clear();
   jetCEF_                                 .clear();
   jetNEF_                                 .clear();
   jetNCH_                                 .clear();
   jetHFHAE_                               .clear();
   jetHFEME_                               .clear();
   jetNConstituents_                       .clear();   
   // Set event level quantities

   totalET = 0;
   HT = 0;
   nJets = 0;
   nGoodJets = 0;

   // Compute HT
   
   for(uint32_t j = 0; j < pfJets->size(); j++) {
     const pat::Jet &jet = (*pfJets)[j];
     if(j == 0) {
       j1PT = jet.pt();
       j1Eta = jet.eta();
       j1Phi = jet.phi();
       j1CHdFr = jet.chargedHadronEnergyFraction();
       j1NHdFr = jet.neutralHadronEnergyFraction();
       j1CEmFr = jet.chargedEmEnergyFraction();
       j1NEmFr = jet.neutralEmEnergyFraction();
       j1nCons = jet.nConstituents();
       JetWidthCalculator jwc(jet);
       j1etaWidth = jwc.getEtaWidth();
       j1phiWidth = jwc.getPhiWidth();
       j1etaWidthInECal = jwc.getEtaWidthInECal();
       j1phiWidthInECal = jwc.getPhiWidthInECal();
       j1etaWidthInHCal = jwc.getEtaWidthInHCal();
       j1phiWidthInHCal = jwc.getPhiWidthInHCal();
     }
     else if(j == 1) {
       j2PT = jet.pt();
       j2Eta = jet.eta();
       j2Phi = jet.phi();
       j2CHdFr = jet.chargedHadronEnergyFraction();
       j2NHdFr = jet.neutralHadronEnergyFraction();
       j2CEmFr = jet.chargedEmEnergyFraction();
       j2NEmFr = jet.neutralEmEnergyFraction();
       j2nCons = jet.nConstituents();
       JetWidthCalculator jwc(jet);
       j2etaWidth = jwc.getEtaWidth();
       j2phiWidth = jwc.getPhiWidth();
       j2etaWidthInECal = jwc.getEtaWidthInECal();
       j2phiWidthInECal = jwc.getPhiWidthInECal();
       j2etaWidthInHCal = jwc.getEtaWidthInHCal();
       j2phiWidthInHCal = jwc.getPhiWidthInHCal();
     }
     if(jet.pt() > 30.) { // Good jets have high pt
       HT += jet.pt();
       nGoodJets++;
     }
     //storing jetpT,jetEta,jetPhi and all other variables
     jetPt_.push_back(jet.pt());
     jetEn_.push_back(jet.energy());
     jetEta_.push_back(jet.eta());  
     jetPhi_.push_back(jet.phi());
     jetCEF_.push_back(   jet.chargedEmEnergyFraction());
     jetNEF_.push_back(   jet.neutralEmEnergyFraction());
     jetCHF_.push_back(   jet.chargedHadronEnergyFraction());
     jetNHF_.push_back(   jet.neutralHadronEnergyFraction());
     jetNCH_.push_back(   jet.chargedMultiplicity());    
     jetHFHAE_.push_back( jet.HFHadronEnergy());
     jetHFEME_.push_back( jet.HFEMEnergy());
     jetNConstituents_.push_back(jet.numberOfDaughters());
     
     totalET += jet.pt(); // Use all jets
     nJets++;
   
     //jet PF Loose ID
     bool jetID = true;
     if (fabs(jet.eta()) <= 3.0) {
      if (!(jet.neutralHadronEnergyFraction() < 0.99))                       jetID = false;
      if (!(jet.neutralEmEnergyFraction() < 0.99))                           jetID = false;
      if (!((jet.chargedMultiplicity() + jet.neutralMultiplicity()) > 1))  jetID = false;
      if (fabs(jet.eta()) <= 2.4) {
        if (!(jet.chargedHadronEnergyFraction() > 0))  jetID = false;
        if (!(jet.chargedMultiplicity() > 0))          jetID = false;
        if (!(jet.chargedEmEnergyFraction() < 0.99))   jetID = false;
      }
    }
    if (fabs(jet.eta()) > 3.0) {
      if (!(jet.neutralEmEnergyFraction() < 0.90))  jetID = false;
      if (!(jet.neutralMultiplicity() > 10))        jetID = false;
    }
    jetPFLooseId_.push_back(jetID);

  }
   
   METValue = (*pfMETs)[0].pt();
   METPhi = (*pfMETs)[0].phi();
   
   std::cout << "TotalET = " << totalET << "; nJets = " << nJets 
	     << "; HT = " << HT << "; nGoodJets = " << nGoodJets 
	     << "; MET = (" << METValue << ", " << METPhi << ")" 
	     << "; nPFCands = " << pfCands->size() << std::endl;

   tree->Fill();
  
}

// ------------ method called once each job just before starting event loop  ------------
void 
JetAnalyzer::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
JetAnalyzer::endJob() 
{
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
JetAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(JetAnalyzer);