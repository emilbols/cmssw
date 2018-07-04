
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/StreamID.h"

#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

#include "DataFormats/BTauReco/interface/ShallowTagInfo.h"

#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"

#include "DataFormats/BTauReco/interface/DeepFlavourTagInfo.h"
#include "DataFormats/BTauReco/interface/DeepFlavourFeatures.h"

#include "RecoBTag/TensorFlow/interface/JetConverter.h"
#include "RecoBTag/TensorFlow/interface/ShallowTagInfoConverter.h"
#include "RecoBTag/TensorFlow/interface/SecondaryVertexConverter.h"
#include "RecoBTag/TensorFlow/interface/NeutralCandidateConverter.h"
#include "RecoBTag/TensorFlow/interface/ChargedCandidateConverter.h"

#include "RecoBTag/TensorFlow/interface/TrackInfoBuilder.h"
#include "RecoBTag/TensorFlow/interface/sorting_modules.h"

#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/Candidate/interface/VertexCompositePtrCandidate.h"
#include "RecoBTag/TensorFlow/interface/deep_helpers.h"

#include "FWCore/ParameterSet/interface/Registry.h"
#include "FWCore/Common/interface/Provenance.h"
#include "DataFormats/Provenance/interface/ProductProvenance.h"

class DeepFlavourTagInfoProducer : public edm::stream::EDProducer<> {

  public:
	  explicit DeepFlavourTagInfoProducer(const edm::ParameterSet&);
	  ~DeepFlavourTagInfoProducer() override;

	  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

  private:
    typedef std::vector<reco::DeepFlavourTagInfo> DeepFlavourTagInfoCollection;
    typedef reco::VertexCompositePtrCandidateCollection SVCollection;
    typedef reco::VertexCollection VertexCollection;
    typedef edm::View<reco::ShallowTagInfo> ShallowTagInfoCollection;

	  void beginStream(edm::StreamID) override {}
	  void produce(edm::Event&, const edm::EventSetup&) override;
	  void endStream() override {}


    const double jet_radius_;
    const double min_candidate_pt_;
    const bool flip_;

    edm::EDGetTokenT<edm::View<reco::Jet>>  jet_token_;
    edm::EDGetTokenT<VertexCollection> vtx_token_;
    edm::EDGetTokenT<SVCollection> sv_token_;
    edm::EDGetTokenT<ShallowTagInfoCollection> shallow_tag_info_token_;
 
};

DeepFlavourTagInfoProducer::DeepFlavourTagInfoProducer(const edm::ParameterSet& iConfig) :
  jet_radius_(iConfig.getParameter<double>("jet_radius")),
  min_candidate_pt_(iConfig.getParameter<double>("min_candidate_pt")),
  flip_(iConfig.getParameter<bool>("flip")),
  jet_token_(consumes<edm::View<reco::Jet> >(iConfig.getParameter<edm::InputTag>("jets"))),
  vtx_token_(consumes<VertexCollection>(iConfig.getParameter<edm::InputTag>("vertices"))),
  sv_token_(consumes<SVCollection>(iConfig.getParameter<edm::InputTag>("secondary_vertices"))),
  shallow_tag_info_token_(consumes<ShallowTagInfoCollection>(iConfig.getParameter<edm::InputTag>("shallow_tag_infos")))

{
    produces<DeepFlavourTagInfoCollection>();


}


DeepFlavourTagInfoProducer::~DeepFlavourTagInfoProducer()
{
}

void DeepFlavourTagInfoProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions)
{
  // pfDeepFlavourTagInfos
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("shallow_tag_infos", edm::InputTag("pfDeepCSVTagInfos"));
  desc.add<double>("jet_radius", 0.4);
  desc.add<double>("min_candidate_pt", 0.95);
  desc.add<bool>("flip", false);
  desc.add<edm::InputTag>("vertices", edm::InputTag("offlinePrimaryVertices"));
  desc.add<edm::InputTag>("secondary_vertices", edm::InputTag("inclusiveCandidateSecondaryVertices"));
  desc.add<edm::InputTag>("jets", edm::InputTag("ak4PFJetsCHS"));
  descriptions.add("pfDeepFlavourTagInfos", desc);
}

void DeepFlavourTagInfoProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{

  auto output_tag_infos = std::make_unique<DeepFlavourTagInfoCollection>();

  edm::Handle<edm::View<reco::Jet>> jets;
  iEvent.getByToken(jet_token_, jets);

  edm::Handle<VertexCollection> vtxs;
  iEvent.getByToken(vtx_token_, vtxs);
  if (vtxs->empty()) {
    // produce empty TagInfos in case no primary vertex
    iEvent.put(std::move(output_tag_infos));
    return; // exit event
  }
  // reference to primary vertex
  const auto & pv = vtxs->at(0);

  edm::Handle<SVCollection> svs;
  iEvent.getByToken(sv_token_, svs);

  edm::Handle<ShallowTagInfoCollection> shallow_tag_infos;
  iEvent.getByToken(shallow_tag_info_token_, shallow_tag_infos);

 
  edm::ESHandle<TransientTrackBuilder> track_builder;
  iSetup.get<TransientTrackRecord>().get("TransientTrackBuilder", track_builder);

  for (std::size_t jet_n = 0; jet_n <  jets->size(); jet_n++) {

    // create data containing structure
    btagbtvdeep::DeepFlavourFeatures features;

    // reco jet reference (use as much as possible)
    const auto & jet = jets->at(jet_n);
    // dynamical casting to pointers, null if not possible
    const auto * pf_jet = dynamic_cast<const reco::PFJet *>(&jet);
    const auto * pat_jet = dynamic_cast<const pat::Jet *>(&jet);
    edm::RefToBase<reco::Jet> jet_ref(jets, jet_n);
    // TagInfoCollection not in an associative container so search for matchs
    const edm::View<reco::ShallowTagInfo> & taginfos = *shallow_tag_infos;
    edm::Ptr<reco::ShallowTagInfo> match;
    // Try first by 'same index'
    if ((jet_n < taginfos.size()) && (taginfos[jet_n].jet() == jet_ref)) {
        match = taginfos.ptrAt(jet_n);
    } else {
      // otherwise fail back to a simple search
      for (auto itTI = taginfos.begin(), edTI = taginfos.end(); itTI != edTI; ++itTI) {
        if (itTI->jet() == jet_ref) { match = taginfos.ptrAt( itTI - taginfos.begin() ); break; }
      }
    }
    reco::ShallowTagInfo tag_info;
    if (match.isNonnull()) {
      tag_info = *match;
    } // will be default values otherwise

    // fill basic jet features
    btagbtvdeep::JetConverter::jetToFeatures(jet, features.jet_features);

    // fill number of pv
    features.npv = vtxs->size();
    math::XYZVector jet_dir = jet.momentum().Unit();
    GlobalVector jet_ref_track_dir(jet.px(),
                                   jet.py(),
                                   jet.pz());

    // fill features from ShallowTagInfo
    const auto & tag_info_vars = tag_info.taggingVariables();
    btagbtvdeep::bTagToFeatures(tag_info_vars, features.tag_info_features);

    // copy which will be sorted
    auto svs_sorted = *svs;
    // sort by dxy
    std::sort(svs_sorted.begin(), svs_sorted.end(),
              [&pv](const auto & sva, const auto &svb)
              { return btagbtvdeep::sv_vertex_comparator(sva, svb, pv); });
    // fill features from secondary vertices
    for (const auto & sv : svs_sorted) {
      if (reco::deltaR2(sv.position() - pv.position(), flip_ ? -jet_dir : jet_dir) > (jet_radius_*jet_radius_)) continue;
      else {
        features.sv_features.emplace_back();
        // in C++17 could just get from emplace_back output
        auto & sv_features = features.sv_features.back();
        btagbtvdeep::svToFeatures(sv, pv, jet, sv_features, flip_);
      }
    }

    // stuff required for dealing with pf candidates

    std::vector<btagbtvdeep::SortingClass<size_t> > c_sorted, n_sorted;

    // to cache the TrackInfo
    std::map<unsigned int, btagbtvdeep::TrackInfoBuilder> trackinfos;

    // unsorted reference to sv
    const auto & svs_unsorted = *svs;
    // fill collection, from DeepTNtuples plus some styling
    for (unsigned int i = 0; i <  jet.numberOfDaughters(); i++){
        auto cand = jet.daughter(i);
        if(cand){
          // candidates under 950MeV (configurable) are not considered
          // might change if we use also white-listing
          if (cand->pt()< min_candidate_pt_) continue;
          if (cand->charge() != 0) {
            auto & trackinfo = trackinfos.emplace(i,track_builder).first->second;
            trackinfo.buildTrackInfo(cand,jet_dir,jet_ref_track_dir,pv);
            c_sorted.emplace_back(i, trackinfo.getTrackSip2dSig(),
                    -btagbtvdeep::mindrsvpfcand(svs_unsorted,cand), cand->pt()/jet.pt());
          } else {
            n_sorted.emplace_back(i, -1,
                    -btagbtvdeep::mindrsvpfcand(svs_unsorted,cand), cand->pt()/jet.pt());
          }
        }
    }

    // sort collections (open the black-box if you please)
    std::sort(c_sorted.begin(),c_sorted.end(),
      btagbtvdeep::SortingClass<std::size_t>::compareByABCInv);
    std::sort(n_sorted.begin(),n_sorted.end(),
      btagbtvdeep::SortingClass<std::size_t>::compareByABCInv);

    std::vector<size_t> c_sortedindices,n_sortedindices;

    // this puts 0 everywhere and the right position in ind
    c_sortedindices=btagbtvdeep::invertSortingVector(c_sorted);
    n_sortedindices=btagbtvdeep::invertSortingVector(n_sorted);

    // set right size to vectors
    features.c_pf_features.clear();
    features.c_pf_features.resize(c_sorted.size());
    features.n_pf_features.clear();
    features.n_pf_features.resize(n_sorted.size());


    const edm::Provenance *prov = shallow_tag_infos.provenance();
    const edm::ParameterSet& psetFromProvenance = edm::parameterSet(*prov);
    double negative_cut = ( ( psetFromProvenance.getParameter<edm::ParameterSet>("computer") 
			      ).getParameter<edm::ParameterSet>("trackSelection") 
			    ).getParameter<double>("sip3dSigMax");

  for (unsigned int i = 0; i <  jet.numberOfDaughters(); i++){

    // get pointer and check that is correct
    auto cand = dynamic_cast<const reco::Candidate *>(jet.daughter(i));
    if(!cand) continue;
    // candidates under 950MeV are not considered
    // might change if we use also white-listing
    if (cand->pt()<0.95) continue;

    auto packed_cand = dynamic_cast<const pat::PackedCandidate *>(cand);
    auto reco_cand = dynamic_cast<const reco::PFCandidate *>(cand);

    // need some edm::Ptr or edm::Ref if reco candidates
    reco::PFCandidatePtr reco_ptr;
    if (pf_jet) {
      reco_ptr = pf_jet->getPFConstituent(i);
    } else if (pat_jet && reco_cand) {
      reco_ptr = pat_jet->getPFConstituent(i);
    }

    float drminpfcandsv = btagbtvdeep::mindrsvpfcand(svs_unsorted, cand);

    if (cand->charge() != 0) {
      // is charged candidate
      auto entry = c_sortedindices.at(i);
      // get cached track info
      auto & trackinfo = trackinfos.at(i);
      if(flip_ && (trackinfo.getTrackSip3dSig() > negative_cut)){continue;}
      // get_ref to vector element
      auto & c_pf_features = features.c_pf_features.at(entry);
      // fill feature structure
      if (packed_cand) {
        btagbtvdeep::packedCandidateToFeatures(packed_cand, jet, trackinfo, 
					       drminpfcandsv, static_cast<float> (jet_radius_), c_pf_features, flip_);
      } else if (reco_cand) {
       
	btagbtvdeep::recoCandidateToFeatures(reco_cand, jet, trackinfo, 
					     drminpfcandsv,  static_cast<float> (jet_radius_),
					     c_pf_features, flip_);
      }
    } else {
      // is neutral candidate
      auto entry = n_sortedindices.at(i);
      // get_ref to vector element
      auto & n_pf_features = features.n_pf_features.at(entry);
      // fill feature structure
      if (packed_cand) {
        btagbtvdeep::packedCandidateToFeatures(packed_cand, jet, drminpfcandsv, static_cast<float> (jet_radius_),
                                                                          n_pf_features);
      } else if (reco_cand) {
        btagbtvdeep::recoCandidateToFeatures(reco_cand, jet, drminpfcandsv, static_cast<float> (jet_radius_),
                                                                        n_pf_features);
      }
    }


  }



  output_tag_infos->emplace_back(features, jet_ref);
  }

  iEvent.put(std::move(output_tag_infos));

}

//define this as a plug-in
DEFINE_FWK_MODULE(DeepFlavourTagInfoProducer);
