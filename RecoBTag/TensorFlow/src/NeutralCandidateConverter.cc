#include "RecoBTag/TensorFlow/interface/NeutralCandidateConverter.h"

namespace btagbtvdeep {
  
  
  
  void packedCandidateToFeatures(const pat::PackedCandidate * n_pf,
				 const pat::Jet & jet,
				 const float drminpfcandsv, const float jetR,
				 NeutralCandidateFeatures & n_pf_features) {
    
    commonCandidateToFeatures(n_pf, jet, drminpfcandsv, jetR, n_pf_features);
    
    n_pf_features.hadFrac = n_pf->hcalFraction();
    
  } 
  
  void recoCandidateToFeatures(const reco::PFCandidate * n_pf,
			       const reco::Jet & jet,
			       const float drminpfcandsv, const float jetR,
			       NeutralCandidateFeatures & n_pf_features) {
    
    commonCandidateToFeatures(n_pf, jet, drminpfcandsv, jetR, n_pf_features);
    
    // need to get a value map and more stuff to do properly
    // otherwise will be different than for PackedCandidates
    // https://github.com/cms-sw/cmssw/blob/master/PhysicsTools/PatAlgos/python/slimming/packedPFCandidates_cfi.py
    if(abs(n_pf->pdgId()) == 1 || abs(n_pf->pdgId()) == 130) {
      n_pf_features.hadFrac = n_pf->hcalEnergy()/(n_pf->ecalEnergy()+n_pf->hcalEnergy());
    } else {
      n_pf_features.hadFrac = 0;
    }
    
  } 
  
  
}

