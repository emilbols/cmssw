
#include "L1Trigger/L1THGCal/interface/fe_codecs/HGCalTriggerCellThresholdCodecImpl.h"


HGCalTriggerCellThresholdCodecImpl::
HGCalTriggerCellThresholdCodecImpl(const edm::ParameterSet& conf) :
    dataLength_(conf.getParameter<uint32_t>("DataLength")),
    nCellsInModule_(conf.getParameter<uint32_t>("MaxCellsInModule")), 
    linLSB_(conf.getParameter<double>("linLSB")),
    linnBits_(conf.getParameter<uint32_t>("linnBits")),
    adcsaturation_(conf.getParameter<double>("adcsaturation")),
    adcnBits_(conf.getParameter<uint32_t>("adcnBits")),
    tdcsaturation_(conf.getParameter<double>("tdcsaturation")), 
    tdcnBits_(conf.getParameter<uint32_t>("tdcnBits")), 
    tdcOnsetfC_(conf.getParameter<double>("tdcOnsetfC")),
    adcsaturationBH_(conf.getParameter<double>("adcsaturationBH")),
    adcnBitsBH_(conf.getParameter<uint32_t>("adcnBitsBH")),
    triggerCellTruncationBits_(conf.getParameter<uint32_t>("triggerCellTruncationBits")),
    TCThreshold_fC_(conf.getParameter<double>("TCThreshold_fC")),
    TCThresholdBH_MIP_(conf.getParameter<double>("TCThresholdBH_MIP")),
    thickness_corrections_(conf.getParameter<std::vector<double>>("ThicknessCorrections"))
{
    adcLSB_ =  adcsaturation_/pow(2.,adcnBits_);
    tdcLSB_ =  tdcsaturation_/pow(2.,tdcnBits_);
    adcLSBBH_ =  adcsaturationBH_/pow(2.,adcnBitsBH_);
    linMax_ = (0x1<<linnBits_)-1;
    triggerCellSaturationBits_ = triggerCellTruncationBits_ + dataLength_;
    TCThreshold_ADC_ = (int) (TCThreshold_fC_ / linLSB_);
    TCThresholdBH_ADC_ = (int) (TCThresholdBH_MIP_ / adcLSBBH_);
}



std::vector<bool>
HGCalTriggerCellThresholdCodecImpl::
encode(const HGCalTriggerCellThresholdCodecImpl::data_type& data, const HGCalTriggerGeometryBase& geometry) const 
{
    // First nCellsInModule_ bits are encoding the map of selected trigger cells
    // Followed by size words of dataLength_ bits, corresponding to energy/transverse energy of
    // the selected trigger cells
  
    // Convert payload into a map for later search
    std::unordered_map<uint32_t, uint32_t> data_map; // (detid,energy)
    size_t size=0;
    for(const auto& triggercell : data.payload)
      {
        data_map.emplace(triggercell.detId(), triggercell.hwPt());
        if (triggercell.hwPt()>0) size++;
      }
    std::vector<bool> result(nCellsInModule_ + dataLength_*size, false);
    // No data: return vector of 0
    if(data.payload.empty()) return result;
    // All trigger cells are in the same module
    // Loop on trigger cell ids in module and check if energy in the cell
    size_t index = 0; // index in module
    size_t idata = 0; // counter for the number of non-zero energy values
    // Retrieve once the ordered list of trigger cells in this module
    uint32_t module = geometry.getModuleFromTriggerCell(data.payload.begin()->detId());
    HGCalTriggerGeometryBase::geom_ordered_set trigger_cells_in_module = geometry.getOrderedTriggerCellsFromModule(module);
    for(const auto& triggercell_id : trigger_cells_in_module)
      {
        // Find if this trigger cell has data
        const auto& data_itr = data_map.find(triggercell_id);
        // if not data, increase index and skip
        if(data_itr==data_map.end())
        {
            index++;
            continue;
        }
        // else fill result vector with data
        // (set the corresponding adress bit and fill energy if >0)
        if(index>=nCellsInModule_) 
        {
          throw cms::Exception("BadGeometry")
            << "Number of trigger cells in module too large for available data payload\n";
        }
        uint32_t value = data_itr->second; 
        if(value>0)
          {
            // Set map bit to 1
            result[index] =  true;
            // Saturate and truncate energy values
            if(value+1>(0x1u<<triggerCellSaturationBits_)) value = (0x1<<triggerCellSaturationBits_)-1;
            for(size_t i=0; i<dataLength_; i++)
              {
                // remove the lowest bits (=triggerCellTruncationBits_)
                result[nCellsInModule_ + idata*dataLength_ + i] = static_cast<bool>(value & (0x1<<(i+triggerCellTruncationBits_)));
              }
            idata++;
          }
        index++;
      }
    return result;
}

HGCalTriggerCellThresholdCodecImpl::data_type 
HGCalTriggerCellThresholdCodecImpl::
decode(const std::vector<bool>& data, const uint32_t module, const HGCalTriggerGeometryBase& geometry) const 
{
    data_type result;
    result.reset();
    // TODO: could eventually reserve result memory to the max size of trigger cells
   
    HGCalTriggerGeometryBase::geom_ordered_set trigger_cells_in_module = geometry.getOrderedTriggerCellsFromModule(module);
    size_t iselected = 0;
    size_t index = 0;
    for(const auto& triggercell : trigger_cells_in_module)
    {
        if(index>=nCellsInModule_)
        {
            throw cms::Exception("BadGeometry")
                << "Number of trigger cells in module too large for available data payload\n";
        }
        if(data[index])
        {
            uint32_t value = 0;
            for(size_t i=0;i<dataLength_;i++)
            {
                size_t ibit = nCellsInModule_+iselected*dataLength_+i; 
                if(data[ibit]) value |= (0x1<<i);
            }
            iselected++;
            // Build trigger cell
            if(value>0)
            {
                // Currently no hardware eta, phi and quality values
                result.payload.emplace_back(reco::LeafCandidate::LorentzVector(),
                        value, 0, 0, 0, triggercell); 
                GlobalPoint point = geometry.getTriggerCellPosition(triggercell);
                // 'value' is hardware, so p4 is meaningless, except for eta and phi
                math::PtEtaPhiMLorentzVector p4((double)value/cosh(point.eta()), point.eta(), point.phi(), 0.);
                result.payload.back().setP4(p4);
                result.payload.back().setPosition(point);
            }
        }
        index++;
    }
    return result;
}


void
HGCalTriggerCellThresholdCodecImpl::
linearize(const std::vector<HGCDataFrame<DetId,HGCSample>>& dataframes,
        std::vector<std::pair<DetId, uint32_t > >& linearized_dataframes)
{
    double amplitude = 0.; 
    uint32_t amplitude_int = 0;
    const int kIntimeSample = 2;

    for(const auto& frame : dataframes) {//loop on DIGI
        if(frame.id().det()==DetId::Forward) {
            if (frame[kIntimeSample].mode()) {//TOT mode
                amplitude =( floor(tdcOnsetfC_/adcLSB_) + 1.0 )* adcLSB_ + double(frame[kIntimeSample].data()) * tdcLSB_;
            }
            else {//ADC mode
                amplitude = double(frame[kIntimeSample].data()) * adcLSB_;
            }

            amplitude_int = uint32_t (floor(amplitude/linLSB_+0.5)); 
        }
        else if(frame.id().det()==DetId::Hcal) {
            // no linearization here. Take the raw ADC data
            amplitude_int = frame[kIntimeSample].data();
        }
        if (amplitude_int>linMax_) amplitude_int = linMax_;

        linearized_dataframes.push_back(std::make_pair (frame.id(), amplitude_int));
    }
}
  

void 
HGCalTriggerCellThresholdCodecImpl::
triggerCellSums(const HGCalTriggerGeometryBase& geometry,  const std::vector<std::pair<DetId, uint32_t > >& linearized_dataframes, data_type& data)
{
    if(linearized_dataframes.empty()) return;
    std::map<HGCalDetId, uint32_t> payload;
    // sum energies in trigger cells
    for(const auto& frame : linearized_dataframes)
    {
        DetId cellid(frame.first);
        // find trigger cell associated to cell
        uint32_t tcid = geometry.getTriggerCellFromCell(cellid);
        HGCalDetId triggercellid( tcid );
        payload.insert( std::make_pair(triggercellid, 0) ); // do nothing if key exists already
        uint32_t value = frame.second; 
        // equalize value among cell thicknesses
        if(cellid.det()==DetId::Forward)
        {
            int thickness = 0;
            switch(cellid.subdetId())
            {
                case ForwardSubdetector::HGCEE:
                    thickness = geometry.eeTopology().dddConstants().waferTypeL(HGCalDetId(cellid).wafer())-1;
                    break;
                case ForwardSubdetector::HGCHEF:
                    thickness = geometry.fhTopology().dddConstants().waferTypeL(HGCalDetId(cellid).wafer())-1;
                    break;
                default:
                    break;
            };
            double thickness_correction = thickness_corrections_.at(thickness);
            value = (double)value*thickness_correction;
        }
        payload[triggercellid] += value; // 32 bits integer should be largely enough 

    }
    uint32_t module = geometry.getModuleFromTriggerCell(payload.begin()->first);
    HGCalTriggerGeometryBase::geom_ordered_set trigger_cells_in_module = geometry.getOrderedTriggerCellsFromModule(module);
    // fill data payload
    for(const auto& id_value : payload)
    {
        // Store only energy value and detid
        // No need position here
        data.payload.emplace_back(reco::LeafCandidate::LorentzVector(),
                        id_value.second, 0, 0, 0, id_value.first.rawId());
    }
}

void 
HGCalTriggerCellThresholdCodecImpl::
thresholdSelect(data_type& data)
{
  for (size_t i = 0; i<data.payload.size();i++){
    int threshold = (HGCalDetId(data.payload[i].detId()).subdetId()==ForwardSubdetector::HGCHEB ? TCThresholdBH_ADC_ : TCThreshold_ADC_);
    if (data.payload[i].hwPt() < threshold)  data.payload[i].setHwPt(0);
  }
  
}


