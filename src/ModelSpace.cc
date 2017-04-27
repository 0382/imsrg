#include "ModelSpace.hh"
#include "AngMom.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <sstream>
#include "omp.h"


using namespace std;

Orbit::~Orbit()
{
//  cout << "In Orbit destructor" << endl;
}

Orbit::Orbit()
: n(-1), l(-1), j2(-1), tz2(-1),occ(-1),cvq(-1),index(-1)
//: n(-1), l(-1), j2(-1), tz2(-1),ph(-1),io(-1),index(-1)
{}

Orbit::Orbit(int n, int l, int j2, int tz2, double occ, int cvq, int index)
: n(n), l(l), j2(j2), tz2(tz2),occ(occ),cvq(cvq),index(index)
//: n(n), l(l), j2(j2), tz2(tz2),ph(ph),io(io),index(index)
{}

Orbit::Orbit(const Orbit& orb)
: n(orb.n), l(orb.l), j2(orb.j2), tz2(orb.tz2),occ(orb.occ),cvq(orb.cvq),index(orb.index)
//: n(orb.n), l(orb.l), j2(orb.j2), tz2(orb.tz2),ph(orb.ph),io(orb.io),index(orb.index)
{}


//************************************************************************
//************************************************************************
//************************************************************************
Ket::~Ket()
{
//  cout << "In Ket destructor" << endl;
}

Ket::Ket()
{}

Ket::Ket(Orbit& op_in, Orbit& oq_in)
: op(&op_in), oq(&oq_in), p(op_in.index), q(oq_in.index)
{
   phase_prefactor = ((op->j2+oq->j2)/2 + 1) % 2==0 ? 1 : -1;
   dpq = p==q ? 1 : 0;
}

int Ket::Phase(int J)
{
   return phase_prefactor * (J%2==0 ? 1 : -1);
}

//************************************************************************
//************************************************************************
//************************************************************************

TwoBodyChannel::~TwoBodyChannel()
{
//  cout << "In TwoBodyChannel destructor" << endl;
}

TwoBodyChannel::TwoBodyChannel()
{}

TwoBodyChannel::TwoBodyChannel(int j, int p, int t, ModelSpace *ms)
{
  Initialize(ms->GetTwoBodyChannelIndex(j,p,t), ms);
}

TwoBodyChannel::TwoBodyChannel(int N, ModelSpace *ms)
{
   Initialize(N,ms);
}

void TwoBodyChannel::Initialize(int N, ModelSpace *ms)
{
   int tbjmax = ms->TwoBodyJmax;
   J = N%(tbjmax+1);
   parity = (N/(tbjmax+1))%2;
   Tz = (N/(2*(tbjmax+1))-1);
   modelspace = ms;
   NumberKets = 0;
   int nk = modelspace->GetNumberKets();
   KetMap.resize(nk,-1); // set all values to -1
   for (int i=0;i<nk;i++)
   {
      Ket &ket = modelspace->GetKet(i);
      if ( CheckChannel_ket(ket) )
      {
         KetMap[i] = NumberKets;
         KetList.push_back(i);
         NumberKets++;
      }
   }
   KetIndex_pp = GetKetIndexFromList(modelspace->KetIndex_pp);
   KetIndex_hh = GetKetIndexFromList(modelspace->KetIndex_hh);
   KetIndex_ph = GetKetIndexFromList(modelspace->KetIndex_ph);
   KetIndex_cc = GetKetIndexFromList(modelspace->KetIndex_cc);
   KetIndex_vc = GetKetIndexFromList(modelspace->KetIndex_vc);
   KetIndex_qc = GetKetIndexFromList(modelspace->KetIndex_qc);
   KetIndex_vv = GetKetIndexFromList(modelspace->KetIndex_vv);
   KetIndex_qv = GetKetIndexFromList(modelspace->KetIndex_qv);
   KetIndex_qq = GetKetIndexFromList(modelspace->KetIndex_qq);
   vector<double> occvec;
   vector<double> unoccvec;
   for (index_t i=0;i<modelspace->KetIndex_hh.size();++i)
   {
      if (CheckChannel_ket(modelspace->GetKet(modelspace->KetIndex_hh[i])))
      {
        occvec.push_back( modelspace->Ket_occ_hh[i]);
        unoccvec.push_back( modelspace->Ket_unocc_hh[i]);
      }
   }
   Ket_occ_hh = arma::vec(occvec);
   Ket_unocc_hh = arma::vec(unoccvec);
   occvec.clear();
   unoccvec.clear();
   for (index_t i=0;i<modelspace->KetIndex_ph.size();++i)
   {
      if (CheckChannel_ket(modelspace->GetKet(modelspace->KetIndex_ph[i])))
      {
        occvec.push_back( modelspace->Ket_occ_ph[i]);
        unoccvec.push_back( modelspace->Ket_unocc_ph[i]);
      }
   }
   Ket_occ_ph = arma::vec(occvec);
   Ket_unocc_ph = arma::vec(unoccvec);
}


//int TwoBodyChannel::GetLocalIndex(int p, int q) const { return KetMap[modelspace->GetKetIndex(p,q)];}; 
int TwoBodyChannel::GetLocalIndex(int p, int q) const
{
 if (p<=q)
   return KetMap[modelspace->GetKetIndex(p,q)];
 else
   return KetMap[modelspace->GetKetIndex(q,p)] + NumberKets;
} 

// get pointer to ket using local index
const Ket & TwoBodyChannel::GetKet(int i) const { return modelspace->GetKet(KetList[i]);}; 
Ket & TwoBodyChannel::GetKet(int i) { return modelspace->GetKet(KetList[i]);}; 


//bool TwoBodyChannel::CheckChannel_ket(int p, int q) const
bool TwoBodyChannel::CheckChannel_ket(Orbit* op, Orbit* oq) const
{
   if ((op->index==oq->index) and (J%2 != 0)) return false; // Pauli principle
   if ((op->l + oq->l)%2 != parity) return false;
   if ((op->tz2 + oq->tz2) != 2*Tz) return false;
   if (op->j2 + oq->j2 < 2*J)       return false;
   if (abs(op->j2 - oq->j2) > 2*J)  return false;

   return true;
}

const arma::uvec& TwoBodyChannel::GetKetIndex_pp() const { return KetIndex_pp;};
const arma::uvec& TwoBodyChannel::GetKetIndex_hh() const { return KetIndex_hh;};
const arma::uvec& TwoBodyChannel::GetKetIndex_ph() const { return KetIndex_ph;};
const arma::uvec& TwoBodyChannel::GetKetIndex_cc() const { return KetIndex_cc;};
const arma::uvec& TwoBodyChannel::GetKetIndex_vc() const { return KetIndex_vc;};
const arma::uvec& TwoBodyChannel::GetKetIndex_qc() const { return KetIndex_qc;};
const arma::uvec& TwoBodyChannel::GetKetIndex_vv() const { return KetIndex_vv;};
const arma::uvec& TwoBodyChannel::GetKetIndex_qv() const { return KetIndex_qv;};
const arma::uvec& TwoBodyChannel::GetKetIndex_qq() const { return KetIndex_qq;};



arma::uvec TwoBodyChannel::GetKetIndexFromList(vector<index_t>& vec_in)
{
   vector<index_t> index_list (min(vec_in.size(),KetList.size()));
   auto it = set_intersection(KetList.begin(),KetList.end(),vec_in.begin(),vec_in.end(),index_list.begin());
   index_list.resize(it-index_list.begin());
   for (auto& x : index_list)
   {
     x = KetMap[x];
   }
   return arma::uvec(index_list);
}

//************************************************************************
//************************************************************************
//************************************************************************

TwoBodyChannel_CC::~TwoBodyChannel_CC()
{
//   cout << "In TwoBodyChannel_CC destructor" << endl;
}

TwoBodyChannel_CC::TwoBodyChannel_CC()
{}

TwoBodyChannel_CC::TwoBodyChannel_CC(int j, int p, int t, ModelSpace *ms)
{
  Initialize(ms->GetTwoBodyChannelIndex(j,p,t), ms);
}

TwoBodyChannel_CC::TwoBodyChannel_CC(int N, ModelSpace *ms)
{
   Initialize(N,ms);
}

// Check if orbits pq participate in this cross-coupled two-body channel
// Difference from regular channels:
// no Pauli rule, <pp||nn> is allowed. But |Tz| is still conserved,
// i.e. <pp||pn> is not allowed. So we use |Tz| rather than Tz,
// and don't use Tz=-1.
bool TwoBodyChannel_CC::CheckChannel_ket(Orbit* op, Orbit* oq) const
{
   if ((op->l + oq->l)%2 != parity)    return false;
   if (abs(op->tz2 + oq->tz2) != 2*Tz) return false;
   if (op->j2 + oq->j2 < 2*J)          return false;
   if (abs(op->j2 - oq->j2) > 2*J)     return false;

   return true;
}


//************************************************************************
//************************************************************************

// Static members

unordered_map<uint64_t,double> ModelSpace::SixJList;
unordered_map<uint64_t,double> ModelSpace::NineJList;
unordered_map<uint64_t,double> ModelSpace::MoshList;
map<string,vector<string>> ModelSpace::ValenceSpaces  {
{ "s-shell"  ,         {"vacuum", "p0s1","n0s1"}},
{ "p-shell"  ,         {"He4", "p0p3","n0p3","p0p1","n0p1"}},
{ "sp-shell"  ,        {"vacuum", "p0s1","n0s1","p0p3","n0p3","p0p1","n0p1"}},
{ "spsd-shell"  ,      {"vacuum", "p0s1","n0s1","p0p3","n0p3","p0p1","n0p1","p0d5","n0d5","p0d3","n0d3","p1s1","n1s1"}},
{ "spsdpf-shell"  ,    {"vacuum", "p0s1","n0s1","p0p3","n0p3","p0p1","n0p1","p0d5","n0d5","p0d3","n0d3","p1s1","n1s1","p0f7","n0f7","p0f5","n0f5","p1p3","n1p3","p1p1","n1p1"}},
{ "sd-shell"  ,        {"O16", "p0d5","n0d5","p0d3","n0d3","p1s1","n1s1"}},
{ "psd-shell"  ,       {"He4", "p0p3","n0p3","p0p1","n0p1","p0d5","n0d5","p0d3","n0d3","p1s1","n1s1"}},
{ "psdNR-shell"  ,     {"He10","p0p3","p0p1","n0d5","n0d3","n1s1"}}, // protons in p shell, neutrons in sd shell (NR is for neutron-rich)
{ "psdPR-shell"  ,     {"O10","n0p3","n0p1","p0d5","p0d3","p1s1"}}, // neutrons in p shell, protnons in sd shell (PR is for proton-rich)
{ "fp-shell"  ,        {"Ca40","p0f7","n0f7","p0f5","n0f5","p1p3","n1p3","p1p1","n1p1"}},
{ "sdfp-shell"  ,      {"O16", "p0d5","n0d5","p0d3","n0d3","p1s1","n1s1","p0f7","n0f7","p0f5","n0f5","p1p3","n1p3","p1p1","n1p1"}},
{ "sdfpNR-shell"  ,    {"O28", "p0d5","p0d3","p1s1","n0f7","n0f5","n1p3","n1p1"}}, // protons in sd shell, neutrons in fp shell, a la SDPFU from Nowacki/Poves (NR is for neutron-rich)
{ "sdfpPR-shell"  ,    {"Ca28", "n0d5","n0d3","n1s1","p0f7","p0f5","p1p3","p1p1"}}, // neutrons in sd shell, protons in fp shell, (PR is for proton-rich)
{ "fpg9-shell"  ,      {"Ca40","p0f7","n0f7","p0f5","n0f5","p1p3","n1p3","p1p1","n1p1","p0g9","n0g9"}},
{ "fpg9NR-shell"  ,    {"Ca40","p0f7","n0f7","p0f5","n0f5","p1p3","n1p3","p1p1","n1p1","n0g9"}}, // just add g9/2 for neutrons
{ "fpgdsNR-shell"  ,   {"Ca60","p0f7","p0f5","p1p3","p1p1","n0g9","n0g7","n1d5","n1d3","n2s1"}}, // protons in the fp shell, neutrons in the gds shell
//{ "fpg9NR-shell"  ,    {"Ca52","p0f7","p0f5","p1p3","p1p1","n0f5","n1p1","n0g9"}}, // protons in the fp shell, neutrons in upper fp + g9/2
{ "sd3f7p3-shell"  ,   {"Si28","p0d3","n0d3","p1s1","n1s1","p0f7","n0f7","p1p3","n1p3"}},
{ "gds-shell" ,        {"Zr80","p0g9","n0g9","p0g7","n0g7","p1d5","n1d5","p1d3","n1d3","p2s1","n2s1"}}, // This is a big valence space, more than a few particles will be a serious shell model diagonalization
};



ModelSpace::~ModelSpace()
{
//  cout << "In ModelSpace destructor. emax = " << Emax << endl;
}

ModelSpace::ModelSpace()
:  Emax(0), E2max(0), E3max(0), Lmax2(0), Lmax3(0), OneBodyJmax(0), TwoBodyJmax(0), ThreeBodyJmax(0), norbits(0),
  hbar_omega(20), target_mass(16),sixj_has_been_precalculated(false), moshinsky_has_been_precalculated(false),
  scalar_transform_first_pass(true), tensor_transform_first_pass(40,true)
{
  cout << "In default constructor" << endl;
}


ModelSpace::ModelSpace(const ModelSpace& ms)
 :
   holes( ms.holes), particles( ms.particles),
   core(ms.core), valence(ms.valence), qspace( ms.qspace), 
   proton_orbits( ms.proton_orbits),neutron_orbits( ms.neutron_orbits),
   KetIndex_pp( ms.KetIndex_pp), KetIndex_ph( ms.KetIndex_ph), KetIndex_hh( ms.KetIndex_hh),
   KetIndex_cc( ms.KetIndex_cc),
   KetIndex_vc( ms.KetIndex_vc),
   KetIndex_qc( ms.KetIndex_qc),
   KetIndex_vv( ms.KetIndex_vv),
   KetIndex_qv( ms.KetIndex_qv),
   KetIndex_qq( ms.KetIndex_qq),
   Ket_occ_hh( ms.Ket_occ_hh),
   Ket_unocc_hh( ms.Ket_unocc_hh),
   Emax(ms.Emax), E2max(ms.E2max), E3max(ms.E3max), Lmax2(ms.Lmax2), Lmax3(ms.Lmax3),
   OneBodyJmax(ms.OneBodyJmax), TwoBodyJmax(ms.TwoBodyJmax), ThreeBodyJmax(ms.ThreeBodyJmax),
   OneBodyChannels(ms.OneBodyChannels),
   SortedTwoBodyChannels(ms.SortedTwoBodyChannels),
   SortedTwoBodyChannels_CC(ms.SortedTwoBodyChannels_CC),
   norbits(ms.norbits), hbar_omega(ms.hbar_omega),
   target_mass(ms.target_mass), target_Z(ms.target_Z), Aref(ms.Aref), Zref(ms.Zref),
   nTwoBodyChannels(ms.nTwoBodyChannels),
   Orbits(ms.Orbits), Kets(ms.Kets),
   TwoBodyChannels(ms.TwoBodyChannels), TwoBodyChannels_CC(ms.TwoBodyChannels_CC),
   PandyaLookup(ms.PandyaLookup),
   sixj_has_been_precalculated(ms.sixj_has_been_precalculated),
   moshinsky_has_been_precalculated(ms.moshinsky_has_been_precalculated),
   scalar_transform_first_pass(true), tensor_transform_first_pass(40,true)
{
   for (TwoBodyChannel& tbc : TwoBodyChannels)   tbc.modelspace = this;
   for (TwoBodyChannel_CC& tbc_cc : TwoBodyChannels_CC)   tbc_cc.modelspace = this;
}

ModelSpace::ModelSpace(ModelSpace&& ms)
 :
   holes( move(ms.holes)), particles( move(ms.particles)),
   core(move(ms.core)), valence(move(ms.valence)),  qspace( move(ms.qspace)),  
   proton_orbits( move(ms.proton_orbits)),
   neutron_orbits( move(ms.neutron_orbits)),
   KetIndex_pp( move(ms.KetIndex_pp)), KetIndex_ph( move(ms.KetIndex_ph)), KetIndex_hh( move(ms.KetIndex_hh)),
   KetIndex_cc( ms.KetIndex_cc),
   KetIndex_vc( ms.KetIndex_vc),
   KetIndex_qc( ms.KetIndex_qc),
   KetIndex_vv( ms.KetIndex_vv),
   KetIndex_qv( ms.KetIndex_qv),
   KetIndex_qq( ms.KetIndex_qq),
   Ket_occ_hh( ms.Ket_occ_hh),
   Ket_unocc_hh( ms.Ket_unocc_hh),
   Emax(ms.Emax), E2max(ms.E2max), E3max(ms.E3max), Lmax2(ms.Lmax2), Lmax3(ms.Lmax3),
   OneBodyJmax(ms.OneBodyJmax), TwoBodyJmax(ms.TwoBodyJmax), ThreeBodyJmax(ms.ThreeBodyJmax),
   OneBodyChannels(move(ms.OneBodyChannels)),
   SortedTwoBodyChannels(move(ms.SortedTwoBodyChannels)),
   SortedTwoBodyChannels_CC(move(ms.SortedTwoBodyChannels_CC)),
   norbits(ms.norbits), hbar_omega(ms.hbar_omega),
   target_mass(ms.target_mass), target_Z(ms.target_Z), Aref(ms.Aref), Zref(ms.Zref),
   nTwoBodyChannels(ms.nTwoBodyChannels),
   Orbits(move(ms.Orbits)), Kets(move(ms.Kets)),
   TwoBodyChannels(move(ms.TwoBodyChannels)), TwoBodyChannels_CC(move(ms.TwoBodyChannels_CC)),
   PandyaLookup(ms.PandyaLookup),
   sixj_has_been_precalculated(ms.sixj_has_been_precalculated),
   moshinsky_has_been_precalculated(ms.moshinsky_has_been_precalculated),
   scalar_transform_first_pass(true), tensor_transform_first_pass(40,true)
{
   for (TwoBodyChannel& tbc : TwoBodyChannels)   tbc.modelspace = this;
   for (TwoBodyChannel_CC& tbc_cc : TwoBodyChannels_CC)   tbc_cc.modelspace = this;
   for (TwoBodyChannel& tbc : ms.TwoBodyChannels)   tbc.modelspace = NULL;
   for (TwoBodyChannel_CC& tbc_cc : ms.TwoBodyChannels_CC)   tbc_cc.modelspace = NULL;
}


// orbit string representation is e.g. p0f7
// Assumes that the core is hole states that aren't in the valence space.
ModelSpace::ModelSpace(int emax, vector<string> hole_list, vector<string> valence_list)
:  Emax(emax), E2max(2*emax), E3max(3*emax), Lmax2(emax), Lmax3(emax), OneBodyJmax(0), TwoBodyJmax(0), ThreeBodyJmax(0), norbits(0), hbar_omega(20), target_mass(16),
     moshinsky_has_been_precalculated(false), scalar_transform_first_pass(true), tensor_transform_first_pass(40,true)
{
   Init(emax, hole_list, hole_list, valence_list); 
}

// If we don't want the reference to be the core
ModelSpace::ModelSpace(int emax, vector<string> hole_list, vector<string> core_list, vector<string> valence_list)
: Emax(emax), E2max(2*emax), E3max(3*emax), Lmax2(emax), Lmax3(emax), OneBodyJmax(0), TwoBodyJmax(0), ThreeBodyJmax(0), norbits(0), hbar_omega(20), target_mass(16),
     sixj_has_been_precalculated(false),moshinsky_has_been_precalculated(false), scalar_transform_first_pass(true), tensor_transform_first_pass(40,true)
{
   Init(emax, hole_list, core_list, valence_list); 
}

// Most conventient interface
ModelSpace::ModelSpace(int emax, string reference, string valence)
: Emax(emax), E2max(2*emax), E3max(3*emax), Lmax2(emax), Lmax3(emax), OneBodyJmax(0), TwoBodyJmax(0), ThreeBodyJmax(0),hbar_omega(20),
     sixj_has_been_precalculated(false),moshinsky_has_been_precalculated(false), scalar_transform_first_pass(true), tensor_transform_first_pass(40,true)
{
  Init(emax,reference,valence);
}

ModelSpace::ModelSpace(int emax, string valence)
: Emax(emax), E2max(2*emax), E3max(3*emax), Lmax2(emax), Lmax3(emax), OneBodyJmax(0), TwoBodyJmax(0), ThreeBodyJmax(0),hbar_omega(20),
     sixj_has_been_precalculated(false),moshinsky_has_been_precalculated(false), scalar_transform_first_pass(true), tensor_transform_first_pass(40,true)
{
  auto itval = ValenceSpaces.find(valence);
  if ( itval != ValenceSpaces.end() ) // we've got a valence space
     Init(emax,itval->second[0],valence);
  else  // no valence space. we've got a single-reference.
     Init(emax,valence,valence);
}



// Specify the reference and either the core or valence
// This is the most convenient interface
void ModelSpace::Init(int emax, string reference, string valence)
{
//  int Aref,Zref;
  GetAZfromString(reference,Aref,Zref);
  map<index_t,double> hole_list = GetOrbitsAZ(Aref,Zref);
  Init(emax,hole_list,valence);
}

void ModelSpace::Init(int emax, map<index_t,double> hole_list, string valence)
{
  int Ac,Zc;
  vector<index_t> valence_list, core_list;

  if (valence == "0hw-shell")
  {
    Get0hwSpace(Aref,Zref,core_list,valence_list);
  }
  else if ( valence.find(",")!=string::npos ) // interpet as a comma-separated list of core followed by valence orbits
  {
    ParseCommaSeparatedValenceSpace(valence,core_list,valence_list);
  }
  else if ( valence.find("FCI")!=string::npos ) // FCI space, so no core, all orbits are valence.
  {
    index_t num_orbits = (emax+1)*(emax+2);
    for (index_t i=0;i<num_orbits;++i) valence_list.push_back( i );
  }
  else // check if it's one of the pre-defined spaces
  {
    auto itval = ValenceSpaces.find(valence);
    string core_string;
  
    if ( itval != ValenceSpaces.end() ) // we've got a valence space
    {
       core_string = itval->second[0];
       valence_list = String2Index(vector<string>(itval->second.begin()+1,itval->second.end()));
    }
    else  // no valence space. we've got a single-reference.
    {
       core_string = valence;
    }
  
    GetAZfromString(core_string,Ac,Zc);
    for (auto& it_core : GetOrbitsAZ(Ac,Zc) ) core_list.push_back(it_core.first);
  }

  target_mass = Aref;
  target_Z = Zref;
  Init(emax,hole_list,core_list,valence_list);
  
}


// Specify the model space with strings of orbit lists.
// Less convenient, but more flexible
void ModelSpace::Init(int emax, vector<string> hole_list, vector<string> core_list, vector<string> valence_list)
{
   cout << "Creating a model space with Emax = " << Emax << "  and hole orbits [";
   for (auto& h : hole_list)  cout << h << " ";
   cout << "]   and core orbits [";
   for (auto& c : core_list)    cout << c << " ";
   cout << "]   and valence orbits [";
   for (auto& v : valence_list)   cout << v << " ";
   cout << "]" << endl;
   map<index_t,double> hole_map;
   for (auto& h : String2Index(hole_list)) hole_map[h] = 1.0;
  Init(emax, hole_map, String2Index(core_list), String2Index(valence_list) );
}


void ModelSpace::Init_occ_from_file(int emax, string valence, string occ_file)
{
  index_t orb;
  double occ;
  map<index_t,double> hole_list;

  ifstream infile(occ_file);
  if (!infile.good())
  {
    cout << endl << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
    cout << "Trouble reading file: " << occ_file << endl;
    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl << endl;
  }

  while( infile >> orb >> occ )
  {
    if ( hole_list.find(orb) != hole_list.end() and  abs( hole_list[orb] -occ) > 1e-6) // the minus sign is for a test. Change it back.
    {
        cout << "Warning: in file " << occ_file << ", redefinition of occupation of orbit "
             << orb << "  " << hole_list[orb] << " => " << occ << endl;
    }
    cout << "from occ file: " << endl;
    hole_list[orb] = occ;
    cout << orb << " " << occ << endl;
  }

  Init(emax,hole_list,valence);
}


// This is the Init which should inevitably be called
void ModelSpace::Init(int emax, map<index_t,double> hole_list, vector<index_t> core_list, vector<index_t> valence_list)
{
   ClearVectors();
   emax = Emax;
   cout << "core list: ";
   for (auto& c : core_list) cout << c << " ";
   cout << endl;
   cout << "valence list: ";
   for (auto& v : valence_list) cout << v << " ";
   cout << endl;
   cout << "hole list: ";
   for (auto& h : hole_list) cout << h.first << " ( " << h.second << " ) ";
   cout << endl;

   // Make sure no orbits are both core and valence
   for (auto& c : core_list)
   {
     if ( find(valence_list.begin(), valence_list.end(), c) != valence_list.end() )
       cout << "!!!!!!!!!!!!! ModelSpace::Init : Conflicting definition. Orbit " << c << " is in core and valence spaces." << endl;
   }

   norbits = (Emax+1)*(Emax+2);
   Orbits.resize(norbits);
   for (int N=0; N<=Emax; ++N)
   {
     for (int l=N; l>=0; l-=2)
     {
       int n = (N-l)/2;
       for (int j2=2*l+1; j2>=2*l-1 and j2>0; j2-=2)
       {
         for (int tz : {-1, 1} )
         {
            double occ = 0;
            int cvq = 2;
            int indx = Index1(n,l,j2,tz);
            if (hole_list.find(indx) != hole_list.end()) occ = hole_list[indx];
            if ( find(core_list.begin(), core_list.end(), indx) != core_list.end() ) cvq=0; // core orbit
            if ( find(valence_list.begin(), valence_list.end(), indx) != valence_list.end() ) cvq=1; // valence orbit
            AddOrbit(n,l,j2,tz,occ,cvq);
         }
       }
     }
   }
   Aref = 0;
   Zref = 0;
   for (auto& h : holes)
   {
     Orbit& oh = GetOrbit(h);
     Aref += (oh.j2+1)*oh.occ;
     if (oh.tz2 < 0) Zref += (oh.j2+1)*oh.occ;
   }
   SetupKets();
}



// Get vector of orbit indices from vector of strings
// e.g. "p0f7" gives the index of the proton 0f7/2 orbit.
vector<index_t> ModelSpace::String2Index( vector<string> vs )
{
  vector<index_t> vi;
  vector<char> l_list = {'s','p','d','f','g','h','i','j','k','l','m','n','o'};

  for ( auto& s : vs )
  {
    int n,l,j2,tz2;
    tz2 = s[0]=='p' ? -1 : 1;
    istringstream( s.substr(1,2) ) >> n;
    l = find(l_list.begin(),l_list.end(), s[2]) - l_list.begin();
    istringstream( s.substr(3,s.size()) ) >> j2;
    vi.push_back( Index1(n,l,j2,tz2) );
  }
  return vi;
}


string ModelSpace::Index2String( index_t ind)
{
  vector<char> l_list = {'s','p','d','f','g','h','i','j','k','l','m','n','o'};
  Orbit& oi = GetOrbit(ind);
  char c[10];
  char pn = oi.tz2 < 0 ? 'p' : 'n';
  char lstr = l_list[oi.l];
  sprintf(c, "%c%d%c%d",pn,oi.n,lstr,oi.j2);
  return string(c) ;
}


void ModelSpace::GetAZfromString(string str,int& A, int& Z) // TODO: accept different formats, e.g. 22Na vs Na22
{
  vector<string> periodic_table = {"n","H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S","Cl","Ar",
                        "K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr",
                        "Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I","Xe",
                        "Cs","Ba","La","Ce","Pr","Nd","Pm","Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf",
                        "Ta","W","Re","Os","Ir","Pt","Au","Hg","Tl","Pb"};
  if (str == "vacuum") str="n0";
  int i=0;
  while (! isdigit(str[i])) i++;
  string elem = str.substr(0,i);
  stringstream( str.substr(i,str.size()-i)) >> A;
  auto it_elem = find(periodic_table.begin(),periodic_table.end(),elem);
  if (it_elem != periodic_table.end())
  {
    Z = it_elem - periodic_table.begin();
  }
  else
  {
    Z =-1;
   cout << "ModelSpace::GetAZfromString :  Trouble parsing " << str << endl;
  }
}

// Fill A orbits with Z protons and A-Z neutrons
// assuming a standard shell-model level ordering
map<index_t,double> ModelSpace::GetOrbitsAZ(int A, int Z)
{
  int zz = 0;
  int nn = 0; // unfortunate there are so many n's here...
  map<index_t,double> holesAZ;
  for (int N=0; N<=Emax; ++N)
  {
    for (int g=2*N+1;g>=-2*N;g-=4)
    {
      int j2 = abs(g);
      int l = g<0 ? (j2+1)/2 : (j2-1)/2;
      int n = (N-l)/2;

      if (zz < Z)
      {
        int dz = min(Z-zz,j2+1);
        holesAZ[Index1(n,l,j2,-1)] = dz/(j2+1.0);
        zz += dz;
      }
      if (nn < A-Z)
      {
        int dn = min(A-Z-nn,j2+1);
        holesAZ[Index1(n,l,j2,1)] = dn/(j2+1.0);
        nn += dn;
      }
      if (zz==Z and nn==A-Z)
      {
         return holesAZ; // We're all done here.
      }
    }
  }
  cout << "Trouble! Model space not big enough to fill A=" << A << " Z="<< Z << "  emax = " << Emax << endl;
  return holesAZ;

}


/// Find the valence space of one single major oscillator shell each for protons and neutrons (not necessarily
/// the same shell for both) which contains the naive shell-model ground state of the reference.
/// For example, if we want to treat C20, with 6 protons and 14 neutrons, we take the 0p shell for protons
/// and 1s0d shell for neutrons.
void ModelSpace::Get0hwSpace(int Aref, int Zref, vector<index_t>& core_list, vector<index_t>& valence_list)
{
  int Nref = Aref-Zref;
  int OSC_protons=0,OSC_neutrons=0;
  while ( (OSC_protons +1)*(OSC_protons +2)*(OSC_protons +3)/3 <= Zref ) OSC_protons++;
  while ( (OSC_neutrons+1)*(OSC_neutrons+2)*(OSC_neutrons+3)/3 <= Nref ) OSC_neutrons++;

  int Zcore = (OSC_protons )*(OSC_protons +1)*(OSC_protons +2)/3;
  int Ncore = (OSC_neutrons)*(OSC_neutrons+1)*(OSC_neutrons+2)/3;

  for (auto& it_core : GetOrbitsAZ(Zcore+Ncore,Zcore)) core_list.push_back(it_core.first);

  if (Zref>Zcore) // if we have a closed major HO shell of protons, then don't decouple any valence proton orbits
  {
    for (int L=OSC_protons; L>=0; L-=2)
    {
      for (int j2=2*L+1;j2>max(2*L-2,0);j2-=2)
      {
        valence_list.push_back( GetOrbitIndex( (OSC_protons-L)/2, L, j2, -1) );
      }
    }
  }
  if (Nref>Ncore)  // likewise for neutrons
  {
    for (int L=OSC_neutrons; L>=0; L-=2)
    {
      for (int j2=2*L+1;j2>max(2*L-2,0);j2-=2)
      {
        valence_list.push_back( GetOrbitIndex( (OSC_neutrons-L)/2, L, j2, 1) );
      }
    }
  }

}


// Parse a string containing a comma-separated list of core + valence orbits
// eg, the usual sd shell would look like "O16,p0d5,n0d5,p0d3,n0d3,p1s1,n1s1".
// The number of ways to specify a model space is getting a bit out of hand...
void ModelSpace::ParseCommaSeparatedValenceSpace(string valence, vector<index_t>& core_list, vector<index_t>& valence_list)
{
  istringstream ss(valence);
  string orbit_string,core_string;
  getline(ss, core_string, ',');

  int Ac,Zc;
  GetAZfromString(core_string,Ac,Zc);
  for (auto& it_core : GetOrbitsAZ(Ac,Zc) )
  {
    core_list.push_back(it_core.first);
  }

  while(getline(ss, orbit_string, ','))
  {
    valence_list.push_back( String2Index({orbit_string})[0]);
  }
}



void ModelSpace::SetReference(vector<index_t> new_reference)
{
  vector<index_t> c = core;
  vector<index_t> v = valence;
  map<index_t,double> h;
  for (auto r : new_reference) h[r] = 1.0;
  ClearVectors();
  Init(Emax, h,c,v);
}

void ModelSpace::SetReference(map<index_t,double> new_reference)
{
  vector<index_t> c = core;
  vector<index_t> v = valence;
  ClearVectors();
  Init(Emax, new_reference,c,v);
}

void ModelSpace::SetReference(string new_reference)
{
  vector<index_t> c = core;
  vector<index_t> v = valence;
  ClearVectors();
  GetAZfromString(new_reference,Aref,Zref);
  map<index_t,double> h = GetOrbitsAZ(Aref,Zref);
  Init(Emax, h,c,v);
}

ModelSpace ModelSpace::operator=(const ModelSpace& ms)
{
   holes =  ms.holes;
   particles =  ms.particles;
   valence = ms.valence;
   qspace =  ms.qspace;
   core = ms.core;
   proton_orbits =  ms.proton_orbits;
   neutron_orbits =  ms.neutron_orbits;
   KetIndex_pp =  ms.KetIndex_pp;
   KetIndex_ph =  ms.KetIndex_ph;
   KetIndex_hh =  ms.KetIndex_hh;
   KetIndex_cc =  ms.KetIndex_cc;
   KetIndex_vc =  ms.KetIndex_vc;
   KetIndex_qc =  ms.KetIndex_qc;
   KetIndex_vv =  ms.KetIndex_vv;
   KetIndex_qv =  ms.KetIndex_qv;
   KetIndex_qq =  ms.KetIndex_qq;
   Ket_occ_hh  =  ms.Ket_occ_hh;
   Ket_unocc_hh  =  ms.Ket_unocc_hh;
   Emax = ms.Emax;
   E2max = ms.E2max;
   E3max = ms.E3max;
   Lmax2 = ms.Lmax2;
   Lmax3 = ms.Lmax3;
   OneBodyJmax = ms.OneBodyJmax;
   TwoBodyJmax = ms.TwoBodyJmax;
   ThreeBodyJmax = ms.ThreeBodyJmax;
   OneBodyChannels = ms.OneBodyChannels;
   SortedTwoBodyChannels = ms.SortedTwoBodyChannels;
   SortedTwoBodyChannels_CC = ms.SortedTwoBodyChannels_CC;
   norbits = ms.norbits;
   hbar_omega = ms.hbar_omega;
   target_mass = ms.target_mass;
   target_mass = ms.target_Z;
   Aref = ms.Aref;
   Zref = ms.Zref;
   Orbits = ms.Orbits;
   Kets = ms.Kets;
   TwoBodyChannels = ms.TwoBodyChannels;
   TwoBodyChannels_CC = ms.TwoBodyChannels_CC;
   for (TwoBodyChannel& tbc : TwoBodyChannels)   tbc.modelspace = this;
   for (TwoBodyChannel_CC& tbc_cc : TwoBodyChannels_CC)   tbc_cc.modelspace = this;

//   cout << "In copy assignment for ModelSpace" << endl;
   return ModelSpace(*this);
}



ModelSpace ModelSpace::operator=(ModelSpace&& ms)
{
   holes =  move(ms.holes);
   particles =  move(ms.particles);
   valence = move(ms.valence);
   qspace =  move(ms.qspace);
   core = move(ms.core);
   proton_orbits =  move(ms.proton_orbits);
   neutron_orbits =  move(ms.neutron_orbits);
   KetIndex_pp =  move(ms.KetIndex_pp);
   KetIndex_ph =  move(ms.KetIndex_ph);
   KetIndex_hh =  move(ms.KetIndex_hh);
   KetIndex_cc =  move(ms.KetIndex_cc);
   KetIndex_vc =  move(ms.KetIndex_vc);
   KetIndex_qc =  move(ms.KetIndex_qc);
   KetIndex_vv =  move(ms.KetIndex_vv);
   KetIndex_qv =  move(ms.KetIndex_qv);
   KetIndex_qq =  move(ms.KetIndex_qq);
   Ket_unocc_hh =  move(ms.Ket_unocc_hh);
   Ket_occ_hh =  move(ms.Ket_occ_hh);
   Emax = move(ms.Emax);
   E2max = move(ms.E2max);
   E3max = move(ms.E3max);
   Lmax2 = move(ms.Lmax2);
   Lmax3 = move(ms.Lmax3);
   OneBodyJmax = move(ms.OneBodyJmax);
   TwoBodyJmax = move(ms.TwoBodyJmax);
   ThreeBodyJmax = move(ms.ThreeBodyJmax);
   OneBodyChannels = move(ms.OneBodyChannels);
   SortedTwoBodyChannels = move(ms.SortedTwoBodyChannels);
   SortedTwoBodyChannels_CC = move(ms.SortedTwoBodyChannels_CC);
   norbits = move(ms.norbits);
   hbar_omega = move(ms.hbar_omega);
   target_mass = move(ms.target_mass);
   target_Z = move(ms.target_Z);
   Aref = move(ms.Aref);
   Zref = move(ms.Zref);
   Orbits = move(ms.Orbits);
   Kets = move(ms.Kets);
   TwoBodyChannels = move(ms.TwoBodyChannels);
   TwoBodyChannels_CC = move(ms.TwoBodyChannels_CC);
   for (TwoBodyChannel& tbc : TwoBodyChannels)   tbc.modelspace = this;
   for (TwoBodyChannel_CC& tbc_cc : TwoBodyChannels_CC)   tbc_cc.modelspace = this;
   for (TwoBodyChannel& tbc : ms.TwoBodyChannels)   tbc.modelspace = NULL;
   for (TwoBodyChannel_CC& tbc_cc : ms.TwoBodyChannels_CC)   tbc_cc.modelspace = NULL;
   return ModelSpace(*this);
}



void ModelSpace::AddOrbit(Orbit orb)
{
  AddOrbit(orb.n, orb.l, orb.j2, orb.tz2, orb.occ, orb.cvq);
}

void ModelSpace::AddOrbit(int n, int l, int j2, int tz2, double occ, int cvq)
{
   index_t ind = Index1(n, l, j2, tz2);
   Orbits[ind] = Orbit(n,l,j2,tz2,occ,cvq,ind);

   if (j2 > OneBodyJmax)
   {
      OneBodyJmax = j2;
      TwoBodyJmax = OneBodyJmax;
      ThreeBodyJmax = OneBodyJmax*3-1;
      nTwoBodyChannels = 2*3*(TwoBodyJmax+1);
   }

   if ( occ < OCC_CUT) particles.push_back(ind);
   else holes.push_back(ind);
   if (cvq == 0) core.push_back(ind);
   if (cvq == 1) valence.push_back(ind);
   if (cvq == 2) qspace.push_back(ind);
   if (tz2 < 0 ) proton_orbits.push_back(ind);
   if (tz2 > 0 ) neutron_orbits.push_back(ind);

   OneBodyChannels[{l, j2, tz2}].push_back(ind);
}



int ModelSpace::GetOrbitIndex(string orb)
{
  vector<char> l_list = {'s','p','d','f','g','h','i','j','k','l','m','n','o'};
  int n=-1,l=-1,j2=-1;
  int tz2 = orb[0]=='p' ? -1 : 1;
  stringstream(orb.substr(1,1)) >> n;
  auto it_l = find(l_list.begin(), l_list.end(), orb[2]);
  if ( it_l != l_list.end() )
    l = it_l - l_list.begin();
  else
    cout << "Bad orbit label " << orb << endl;
  stringstream(orb.substr(3)) >> j2;
  return Index1(n,l,j2,tz2);
}

int ModelSpace::GetTwoBodyChannelIndex(int j, int p, int t)
{
   return (t+1)*2*(TwoBodyJmax+1) + p*(TwoBodyJmax+1) + j;
}




void ModelSpace::SetupKets()
{
//   nkets = norbits*(norbits+1)/2;
   Kets.resize(Index2(norbits-1,norbits-1)+1);
//   Kets.resize(nkets);
   for (int p=0;p<norbits;p++)
   {
     for (int q=p;q<norbits;q++)
     {
        index_t index = Index2(p,q);
        Kets[index] = Ket(GetOrbit(p),GetOrbit(q));
     }
   }
  for (index_t index=0;index<Kets.size();++index)
  {
    Ket& ket = Kets[index];
    int Tz = (ket.op->tz2 + ket.oq->tz2)/2;
    int parity = (ket.op->l + ket.oq->l)%2;
    MonopoleKets[Tz+1][parity][index] = MonopoleKets[Tz+1][parity].size()-1;
    double occp = ket.op->occ;
    double occq = ket.oq->occ;
    int cvq_p = ket.op->cvq;
    int cvq_q = ket.oq->cvq;
    if (cvq_p+cvq_q==0)      KetIndex_cc.push_back(index); // 00
    if (cvq_p+cvq_q==1)      KetIndex_vc.push_back(index); // 01
    if (abs(cvq_p-cvq_q)==2) KetIndex_qc.push_back(index); // 02
    if (cvq_p*cvq_q==1)      KetIndex_vv.push_back(index); // 11
    if (cvq_p+cvq_q==3)      KetIndex_qv.push_back(index); // 12
    if (cvq_p+cvq_q==4)      KetIndex_qq.push_back(index); // 22
    if (occp<OCC_CUT and occq<OCC_CUT) KetIndex_pp.push_back(index);
//    if (occp>OCC_CUT or occq>OCC_CUT)
    if ( (occp>OCC_CUT) xor (occq>OCC_CUT) )
    {
       KetIndex_ph.push_back(index);
       Ket_occ_ph.push_back(occp*occq);
       Ket_unocc_ph.push_back((1-occp)*(1-occq));
    }
    if (occp>OCC_CUT and occq>OCC_CUT)
    {
       KetIndex_hh.push_back(index);
       Ket_occ_hh.push_back(occp*occq);
       Ket_unocc_hh.push_back((1-occp)*(1-occq));
    }
   }

   SortedTwoBodyChannels.resize(nTwoBodyChannels);
   SortedTwoBodyChannels_CC.resize(nTwoBodyChannels);
   for (int ch=0;ch<nTwoBodyChannels;++ch)
   {
      TwoBodyChannels.emplace_back(TwoBodyChannel(ch,this));
      TwoBodyChannels_CC.emplace_back(TwoBodyChannel_CC(ch,this));
      SortedTwoBodyChannels[ch] = ch;
      SortedTwoBodyChannels_CC[ch] = ch;
   }
   // Sort the two body channels in descending order of matrix dimension and discard the size-0 ones.
   // Hopefully this can help with load balancing.
   sort(SortedTwoBodyChannels.begin(),SortedTwoBodyChannels.end(),[this](int i, int j){ return TwoBodyChannels[i].GetNumberKets() > TwoBodyChannels[j].GetNumberKets(); }  );
   sort(SortedTwoBodyChannels_CC.begin(),SortedTwoBodyChannels_CC.end(),[this](int i, int j){ return TwoBodyChannels_CC[i].GetNumberKets() > TwoBodyChannels_CC[j].GetNumberKets(); }  );
   while (  TwoBodyChannels[ SortedTwoBodyChannels.back() ].GetNumberKets() <1 ) SortedTwoBodyChannels.pop_back();
   while (  TwoBodyChannels_CC[ SortedTwoBodyChannels_CC.back() ].GetNumberKets() <1 ) SortedTwoBodyChannels_CC.pop_back();
}


void ModelSpace::ClearVectors()
{
   holes.clear();         
   particles.clear();     
   core.clear();          
   valence.clear();       
   qspace.clear();        
   proton_orbits.clear();  
   neutron_orbits.clear();
   
   KetIndex_pp.clear();
   KetIndex_ph.clear();
   KetIndex_hh.clear();
   KetIndex_cc.clear();
   KetIndex_vc.clear();
   KetIndex_qc.clear();
   KetIndex_vv.clear();
   KetIndex_qv.clear();
   KetIndex_qq.clear();
   Ket_occ_hh.clear();
   Ket_occ_ph.clear();
   Ket_unocc_hh.clear();
   Ket_unocc_ph.clear();
   for (index_t Tz=0; Tz<3; ++Tz)
   {
     for (index_t parity=0;parity<2; ++parity)
     {
        MonopoleKets[Tz][parity].clear();
     }
   }

   Orbits.clear();
   Kets.clear();
   OneBodyChannels.clear();
   TwoBodyChannels.clear();
   TwoBodyChannels_CC.clear();
   SortedTwoBodyChannels.clear();
   SortedTwoBodyChannels_CC.clear();
   PandyaLookup.clear();
}


// On the first pass through a commutator calculation, we use a single thread so that
// we only calculate the sixJ's we need, but we don't have to worry about race conditions.
// This resets the first pass flag.
void ModelSpace::ResetFirstPass()
{
  scalar_transform_first_pass = true;
  for (size_t i=0;i<tensor_transform_first_pass.size();i++) tensor_transform_first_pass[i] = true;
}

uint64_t ModelSpace::SixJHash(double j1, double j2, double j3, double J1, double J2, double J3)
{
   return   (((uint64_t)(2*j1)) << 50)
          + (((uint64_t)(2*j2)) << 40)
          + (((uint64_t)(2*j3)) << 30)
          + (((uint64_t)(2*J1)) << 20)
          + (((uint64_t)(2*J2)) << 10)
          +  ((uint64_t)(2*J3));

}


void ModelSpace::SixJUnHash(uint64_t key, uint64_t& j1, uint64_t& j2, uint64_t& j3, uint64_t& J1, uint64_t& J2, uint64_t& J3)
{
   j1 = (key >> 50) & 0x3FFL;
   j2 = (key >> 40) & 0x3FFL;
   j3 = (key >> 30) & 0x3FFL;
   J1 = (key >> 20) & 0x3FFL;
   J2 = (key >> 10) & 0x3FFL;
   J3 = (key      ) & 0x3FFL;
}

uint64_t ModelSpace::MoshinskyHash(uint64_t N, uint64_t Lam, uint64_t n, uint64_t lam, uint64_t n1, uint64_t l1, uint64_t n2, uint64_t l2, uint64_t L)
{
   return   (N   << 54)
          + (Lam << 47)
          + (n   << 41)
          + (lam << 34)
          + (n1  << 28)
          + (l1  << 21)
          + (n2  << 15)
          + (l2  << 8 )
          +  L;
}

void ModelSpace::MoshinskyUnHash(uint64_t key,uint64_t& N,uint64_t& Lam,uint64_t& n,uint64_t& lam,uint64_t& n1,uint64_t& l1,uint64_t& n2,uint64_t& l2,uint64_t& L)
{
   N   = (key >> 54) & 0x3FL;
   Lam = (key >> 47) & 0x7FL;
   n   = (key >> 41) & 0x3FL;
   lam = (key >> 34) & 0x7FL;
   n1  = (key >> 28) & 0x3FL;
   l1  = (key >> 21) & 0x7FL;
   n2  = (key >> 15) & 0x3FL;
   l2  = (key >> 8 ) & 0x7FL;
   L   = (key      ) & 0xFFL;
}

double ModelSpace::GetSixJ(double j1, double j2, double j3, double J1, double J2, double J3)
{
// { j1 j2 j3 }
// { J1 J2 J3 }
   uint64_t key = SixJHash(j1,j2,j3,J1,J2,J3);


   const auto it = SixJList.find(key);
   if (it != SixJList.end() ) return it->second;
   double sixj = AngMom::SixJ(j1,j2,j3,J1,J2,J3);
//   printf(" Uh Oh, I shouldn't be here in GetSixJ(%.1f %.1f %.1f %.1f %.1f %.1f).  key =%lx   sixj=%f\n",j1,j2,j3,J1,J2,J3,key,sixj); 
   #pragma omp critical
   {
     SixJList[key] = sixj;
   }
   if (omp_get_num_threads()>1)
      profiler.counter["N_CalcSixJ_in_Parallel_loop"] +=1;
   return sixj;
}



void ModelSpace::PreCalculateSixJ()
{
  if (sixj_has_been_precalculated) return;
  cout << "Precalculating SixJ's" << endl;
  double t_start = omp_get_wtime();
  vector<uint64_t> KEYS;
  for (int a=0; a<GetNumberOrbits(); a+=2)
  {
   Orbit& oa = GetOrbit(a);
   double ja = oa.j2;
   int ea = 2*oa.n + oa.l;
   for (int b=0; b<=a; b+=2)
   {
    Orbit& ob = GetOrbit(b);
    double jb = ob.j2;
    int eb = 2*ob.n + ob.l;
    for (int c=0; c<=b; c+=2)
    {
     Orbit& oc = GetOrbit(c);
     double jc = oc.j2;
     int ec = 2*oc.n + oc.l;
     if (ea+eb+ec>E3max) continue;
     int Jab_min = abs( ja - jb );
     int Jab_max = ja+jb;
     int Jabin_min = min( abs( jc - jb ), abs( jc - ja) );
     int Jabin_max = max( jc+jb, jc+ja);

     for (int Jab=Jab_min; Jab<=Jab_max; Jab+=2)
     {
      for (int Jab_in=Jabin_min; Jab_in<=Jabin_max; Jab_in+=2)
      {
       int J3_min = max( abs(Jab-jc), min(abs(Jab_in-ja),abs(Jab_in-jb)) );
       int J3_max = min( (Jab+jc), max((Jab_in+ja),(Jab_in+jb)) );
       for (int J3=J3_min; J3<=J3_max; J3+=2)
       {
         uint64_t key = SixJHash(0.5*ja,0.5*jb,0.5*Jab,0.5*jc,0.5*J3,0.5*Jab_in);
         if ( SixJList.count(key) == 0 ) 
         {
           KEYS.push_back(key);
           SixJList[key] = 0.; // Make sure eveything's in there to avoid a rehash in the parallel loop
         }
         key = SixJHash(0.5*jb,0.5*ja,0.5*Jab,0.5*jc,0.5*J3,0.5*Jab_in);
         if ( SixJList.count(key) == 0 ) 
         {
           KEYS.push_back(key);
           SixJList[key] = 0.; // Make sure eveything's in there to avoid a rehash in the parallel loop
         }

       }
       for (int Lambda=abs(Jab-Jab_in); Lambda<=(Jab+Jab_in); Lambda+=2)
       {
         uint64_t  key = SixJHash(0.5*jb,0.5*ja,0.5*Jab,0.5*jc,0.5*Lambda,0.5*Jab_in);
         if ( SixJList.count(key) == 0 ) 
         {
           KEYS.push_back(key);
           SixJList[key] = 0.; // Make sure eveything's in there to avoid a rehash in the parallel loop
         }
         key = SixJHash(0.5*Jab,0.5*Lambda,0.5*Jab_in,0.5*jb,0.5*ja,0.5*jc);
         if ( SixJList.count(key) == 0 ) 
         {
           KEYS.push_back(key);
           SixJList[key] = 0.; // Make sure eveything's in there to avoid a rehash in the parallel loop
         }
       }
      }
     }
    }
   }
  }
  #pragma omp parallel for schedule(dynamic,1)
  for (size_t i=0;i< KEYS.size(); ++i)
  {
//    unsigned long long int& key = KEYS[i];
    uint64_t j1,j2,j3,J1,J2,J3;
    uint64_t key = KEYS[i];
    SixJUnHash(key, j1,j2,j3,J1,J2,J3);
//    int j1 = (key >> 30) & 0x3F;
//    int j2 = (key >> 24) & 0x3F;
//    int j3 = (key >> 18) & 0x3F;
//    int J1 = (key >> 12) & 0x3F;
//    int J2 = (key >>  6) & 0x3F;
//    int J3 = (key      ) & 0x3F;
    SixJList[key] = AngMom::SixJ(0.5*j1,0.5*j2,0.5*j3,0.5*J1,0.5*J2,0.5*J3);
  }
  sixj_has_been_precalculated = true;
  cout << "done calculating sixJs (" << KEYS.size() << " of them)" << endl;
  cout << "Hash table has " << SixJList.bucket_count() << " buckets and a load factor " << SixJList.load_factor() 
       << "  estimated storage ~ " << ((SixJList.bucket_count()+SixJList.size()) * (sizeof(size_t)+sizeof(void*))) / (1024.*1024.*1024.) << " GB" << endl;
  profiler.timer["PreCalculateSixJ"] += omp_get_wtime() - t_start;
}




void ModelSpace::PreCalculateMoshinsky()
{
  if (moshinsky_has_been_precalculated) return;
  double t_start = omp_get_wtime();

  // generating all the keys is fast, so we do this first without parallelization
//  vector<unsigned long long int> KEYS;
  vector<uint64_t> KEYS;
  for (int N=0; N<=E2max/2; ++N)
  {
   for (int n=0; n<=min(N,E2max/2-N); ++n)
   {
    for (int Lam=0; Lam<=E2max-2*N-2*n; ++Lam)
    {
     int lam_max = (N==n ? min(Lam,E2max-2*N-2*n-Lam) : E2max-2*N-2*n-Lam);
     for (int lam=0; lam<=lam_max; ++lam)
     {
      int e2 = 2*N+Lam + 2*n+lam;
      for (int L=abs(Lam-lam); L<=Lam+lam; ++L)
      {
       for (int n1=0; n1<=N; ++n1)
       {
        for (int n2=0; n2<=min(n1,e2/2-n1); ++n2)
        {
         int l1max = n1==N? min(Lam,e2-2*n1-2*n2) : e2-2*n1-2*n2;
         for (int l1=0; l1<=l1max; ++l1 )
         {
          int l2 = e2-2*n1-2*n2-l1;
          if ( (l1+l2+lam+Lam)%2 >0 ) continue;
          if ( l2<abs(L-l1) or l2>L+l1 ) continue;
          // emax = 16, lmax = 32 -> good up to emax=32, which I'm nowhere near.
//          unsigned long long int key =   ((unsigned long long int) N   << 40)
//                                       + ((unsigned long long int) Lam << 34)
//                                       + ((unsigned long long int) n   << 30)
//                                       + ((unsigned long long int) lam << 26)
//                                       + ((unsigned long long int) n1  << 22)
//                                       + ((unsigned long long int) l1  << 16)
//                                       + ((unsigned long long int) n2  << 12)
//                                       + ((unsigned long long int) l2  << 6 )
//                                       +  L;
          uint64_t key = MoshinskyHash(N,Lam,n,lam,n1,l1,n2,l2,L);
//          uint64_t key =    ((uint64_t) N   << 40)
//                          + ((uint64_t) Lam << 34)
//                          + ((uint64_t) n   << 30)
//                          + ((uint64_t) lam << 26)
//                          + ((uint64_t) n1  << 22)
//                          + ((uint64_t) l1  << 16)
//                          + ((uint64_t) n2  << 12)
//                          + ((uint64_t) l2  << 6 )
//                          +  L;
          KEYS.push_back(key);
          MoshList[key] = 0.; // Make sure eveything's in there to avoid a rehash in the parallel loop
         }
        }
       }
      }
     }
    }
   }
  }
  // Now we calculate the Moshinsky brackets in parallel
//  vector<double> mosh_vals( KEYS.size() );
  #pragma omp parallel for schedule(dynamic,1)
  for (size_t i=0;i< KEYS.size(); ++i)
  {
//    unsigned long long int& key = KEYS[i];
    uint64_t key = KEYS[i];
    uint64_t N,Lam,n,lam,n1,l1,n2,l2,L;
    MoshinskyUnHash(key,N,Lam,n,lam,n1,l1,n2,l2,L);
//    int N   =  key >> 40;
//    int Lam = (key >> 34) & 0x3f;
//    int n   = (key >> 30) & 0xf;
//    int lam = (key >> 26) & 0xf;
//    int n1  = (key >> 22) & 0xf;
//    int l1  = (key >> 16) & 0x3f;
//    int n2  = (key >> 12) & 0xf;
//    int l2  = (key >> 6 ) & 0xf;
//    int L   =  key & 0x3f;
    MoshList[key] = AngMom::Moshinsky(N,Lam,n,lam,n1,l1,n2,l2,L);
  }

  moshinsky_has_been_precalculated = true;
  cout << "done calculating moshinsky" << endl;
  cout << "Hash table has " << MoshList.bucket_count() << " buckets and a load factor " << MoshList.load_factor() 
       << "  estimated storage ~ " << ((MoshList.bucket_count()+MoshList.size()) * (sizeof(size_t)+sizeof(void*))) / (1024.*1024.*1024.) << " GB" << endl;
  profiler.timer["PreCalculateMoshinsky"] += omp_get_wtime() - t_start;
}



double ModelSpace::GetMoshinsky( int N, int Lam, int n, int lam, int n1, int l1, int n2, int l2, int L)
{
   int phase_mosh = 1;
  int switches = 10;

  while (switches > 0)
  {
   switches = 0;
   if (n2>n1 or (n2==n1 and l2>l1))
   {
      swap(n1,n2);
      swap(l1,l2);
      phase_mosh *= phase(Lam+L);
      ++switches;
   }
   if (n>N or (n==N and lam>Lam))
   {
      swap(n,N);
      swap(lam,Lam);
      phase_mosh *= phase(l1 +L);
      ++switches;
   }

   if (n1>N or (n1==N and l1>Lam) or (n1==N and l1==Lam and n2>n) or (n1==N and l1==Lam and n2==n and l2>lam) )
   {
      swap(n1,N);
      swap(l1,Lam);
      swap(n2,n);
      swap(l2,lam);
      ++switches;
//      phase_mosh *= phase(l2+lam); // This phase is given in Moshinsky and Brody, but with the current algorithm, it appears not to be required.
   }
  }

     uint64_t key = MoshinskyHash(N,Lam,n,lam,n1,l1,n2,l2,L);
//          unsigned long long int key =   ((unsigned long long int) N   << 40)
//                                       + ((unsigned long long int) Lam << 34)
//                                       + ((unsigned long long int) n   << 30)
//                                       + ((unsigned long long int) lam << 26)
//                                       + ((unsigned long long int) n1  << 22)
//                                       + ((unsigned long long int) l1  << 16)
//                                       + ((unsigned long long int) n2  << 12)
//                                       + ((unsigned long long int) l2  << 6 )
//                                       +  L;


   auto it = MoshList.find(key);
   if ( it != MoshList.end() )  return it->second * phase_mosh;

   // if we didn't find it, we need to calculate it.
   double mosh = AngMom::Moshinsky(N,Lam,n,lam,n1,l1,n2,l2,L);
//   #pragma omp atomic
   MoshList[key] = mosh;
   return mosh * phase_mosh;

}




double ModelSpace::GetNineJ(double j1, double j2, double J12, double j3, double j4, double J34, double J13, double J24, double J)
{
//   cout << "Calling GetNineJ" << endl;
   int k1 = 2*j1;
   int k2 = 2*j2;
   int K12 = 2*J12;
   int k3 = 2*j3;
   int k4 = 2*j4;
   int K34 = 2*J34;
   int K13 = 2*J13;
   int K24 = 2*J24;
   int K = 2*J;

   array<int,9> klist = {k1,k2,K12,k3,k4,K34,K13,K24,K};
   array<double,9> jlist = {j1,j2,J12,j3,j4,J34,J13,J24,J};
   int imin = min_element(klist.begin(),klist.end()) - klist.begin();
   switch (imin)
   {
      case 0:
       klist = {k4,K34,k3,K24,K,K13,k2,K12,k1};
       jlist = {j4,J34,j3,J24,J,J13,j2,J12,j1};
       break;
      case 1:
       klist = {K13,K,K24,k3,K34,k4,k1,K12,k2};
       jlist = {J13,J,J24,j3,J34,j4,j1,J12,j2};
       break;
      case 2:
       klist = {k3,k4,K34,K13,K24,K,k1,k2,K12};
       jlist = {j3,j4,J34,J13,J24,J,j1,j2,J12};
       break;
      case 3:
       klist = {K12,k2,k1,K,K24,K13,K34,k4,k3};
       jlist = {J12,j2,j1,J,J24,J13,J34,j4,j3};
       break;
      case 4:
       klist = {k1,K12,k2,K13,K,K24,k3,K34,k4};
       jlist = {j1,J12,j2,J13,J,J24,j3,J34,j4};
       break;
      case 5:
       klist = {K13,K24,K,k1,k2,K12,k3,k4,K34};
       jlist = {J13,J24,J,j1,j2,J12,j3,j4,J34};
       break;
      case 6:
       klist = {k2,K12,k1,k4,K34,k3,K24,K,K13};
       jlist = {j2,J12,j1,j4,J34,j3,J24,J,J13};
       break;
      case 7:
       klist = {K12,k1,k2,K34,k3,k4,K,K13,K24};
       jlist = {J12,j1,j2,J34,j3,j4,J,J13,J24};
       break;
      case 8:
       break;
   }

   unsigned long long int key =   klist[0];
//   unsigned long long int factor = 100;
   unsigned long long int factor = 91;
   for (int i=1; i<9; ++i)
   {
      key += klist[i]*factor;
      factor *=91;
//      factor *=100;
   }
   auto it = NineJList.find(key);
   if (it != NineJList.end() )
   {
     return it->second;
   }
   double ninej = AngMom::NineJ(jlist[0],jlist[1],jlist[2],jlist[3],jlist[4],jlist[5],jlist[6],jlist[7],jlist[8]);
   #pragma omp critical
   NineJList[key] = ninej;
   return ninej;

}


//map<array<int,2>,vector<array<int,2>>>& ModelSpace::GetPandyaLookup(int rank_J, int rank_T, int parity)
map<array<int,2>,array<vector<int>,2>>& ModelSpace::GetPandyaLookup(int rank_J, int rank_T, int parity)
{
   CalculatePandyaLookup(rank_J,rank_T,parity);
   return PandyaLookup[{rank_J,rank_T,parity}];

}


// Generate a lookup table of all the channels that depend on a given set of Pandya-transformed channels
// this is used in the 222ph commutators to avoid calculating things that won't be used.
void ModelSpace::CalculatePandyaLookup(int rank_J, int rank_T, int parity)
{
   if (PandyaLookup.find({rank_J, rank_T, parity})!=PandyaLookup.end()) return; 
   cout << "CalculatePandyaLookup( " << rank_J << ", " << rank_T << ", " << parity << ") " << endl;
   double t_start = omp_get_wtime();
//   PandyaLookup[{rank_J,rank_T,parity}] = map<array<int,2>,vector<array<int,2>>>();
   PandyaLookup[{rank_J,rank_T,parity}] = map<array<int,2>,array<vector<int>,2>>();
   auto& lookup = PandyaLookup[{rank_J,rank_T,parity}];

   int ntbc    = TwoBodyChannels.size();
   int ntbc_cc = TwoBodyChannels_CC.size();
   for (int ch_bra_cc = 0; ch_bra_cc<ntbc_cc; ++ch_bra_cc)
   {
     for (int ch_ket_cc = ch_bra_cc; ch_ket_cc<ntbc_cc; ++ch_ket_cc)
     {
//       lookup[{ch_bra_cc,ch_ket_cc}] = vector<array<int,2>>();
       lookup[{ch_bra_cc,ch_ket_cc}] = array<vector<int>,2>(); 
//       lookup[{ch_bra_cc,ch_ket_cc}] = { <vector<int>(), vector<int>() }; 
       lookup.at({ch_bra_cc,ch_ket_cc})[0].reserve(ntbc_cc)  ; 
       lookup.at({ch_bra_cc,ch_ket_cc})[1].reserve(ntbc_cc)  ; 
     }
   }

   #pragma omp parallel for schedule(dynamic,1)
   for (int ch_bra_cc = 0; ch_bra_cc<ntbc_cc; ++ch_bra_cc)
   {
     TwoBodyChannel_CC& tbc_bra_cc = TwoBodyChannels_CC[ch_bra_cc];
     int twoJ_bra_cc = 2*tbc_bra_cc.J;
     for (int ch_ket_cc = ch_bra_cc; ch_ket_cc<ntbc_cc; ++ch_ket_cc)
     {
       TwoBodyChannel_CC& tbc_ket_cc = TwoBodyChannels_CC[ch_ket_cc];
//       lookup[{ch_bra_cc,ch_ket_cc}] = vector<array<int,2>>();
//       vector<int>& bra_list = lookup.at({ch_bra_cc,ch_ket_cc})[0];
//       vector<int>& ket_list = lookup.at({ch_bra_cc,ch_ket_cc})[1];
       vector<int> bra_list,ket_list;
       int twoJ_ket_cc = 2*tbc_ket_cc.J;
       for (int ch_bra=0; ch_bra<ntbc; ++ch_bra)
       {
         TwoBodyChannel& tbc_bra = TwoBodyChannels[ch_bra];
         for (int ch_ket=ch_bra; ch_ket<ntbc; ++ch_ket)
         {
           TwoBodyChannel& tbc_ket = TwoBodyChannels[ch_ket];
           if ( abs(tbc_bra.J-tbc_ket.J)>rank_J ) continue;
           if ( (tbc_bra.J+tbc_ket.J)<rank_J ) continue;
           if ( abs(tbc_bra.Tz-tbc_ket.Tz)>rank_T ) continue;
           if ( (tbc_bra.parity + tbc_ket.parity + parity)%2>0 ) continue;

           bool need_it = false;
           for (int ibra=0; ibra<tbc_bra.GetNumberKets(); ++ibra)
           {
             if (need_it) break;
             const Ket& bra = tbc_bra.GetKet(ibra);
             Orbit& oi = *(bra.op);
             Orbit& oj = *(bra.oq);
             for (int iket=0; iket<tbc_ket.GetNumberKets(); ++iket)
             {
               const Ket& ket = tbc_ket.GetKet(iket);
               Orbit& ok = *(ket.op);
               Orbit& ol = *(ket.oq);
               int j3min = abs(oi.j2-ol.j2);
               int j3max = oi.j2+ol.j2;
               int j4min = abs(ok.j2-oj.j2);
               int j4max = ok.j2+oj.j2;
               if (   (oi.l+ol.l)%2==tbc_bra_cc.parity         and (ok.l+oj.l)%2==tbc_ket_cc.parity
                         and abs(oi.tz2+ol.tz2)==2*tbc_bra_cc.Tz   and abs(ok.tz2+oj.tz2)==2*tbc_ket_cc.Tz
                         and j3min<=twoJ_bra_cc and twoJ_bra_cc<=j3max           and j4min<=twoJ_ket_cc and twoJ_ket_cc<=j4max )
               {
                 need_it=true;
                 break;
               }
               if (   (oi.l+ol.l)%2==tbc_ket_cc.parity         and (ok.l+oj.l)%2==tbc_bra_cc.parity
                         and abs(oi.tz2+ol.tz2)==2*tbc_ket_cc.Tz   and abs(ok.tz2+oj.tz2)==2*tbc_bra_cc.Tz
                         and j3min<=twoJ_ket_cc and twoJ_ket_cc<=j3max           and j4min<=twoJ_bra_cc and twoJ_bra_cc<=j4max )
               {
                 need_it=true;
                 break;
               }

               j3min = abs(oj.j2-ol.j2);
               j3max = oj.j2+ol.j2;
               j4min = abs(ok.j2-oi.j2);
               j4max = ok.j2+oi.j2;
               if (   (oj.l+ol.l)%2==tbc_bra_cc.parity         and (ok.l+oi.l)%2==tbc_ket_cc.parity
                         and abs(oj.tz2+ol.tz2)==2*tbc_bra_cc.Tz   and abs(ok.tz2+oi.tz2)==2*tbc_ket_cc.Tz
                         and j3min<=twoJ_bra_cc and twoJ_bra_cc<=j3max           and j4min<=twoJ_ket_cc and twoJ_ket_cc<=j4max )
               {
                 need_it=true;
                 break;
               }
               if (   (oj.l+ol.l)%2==tbc_ket_cc.parity         and (ok.l+oi.l)%2==tbc_bra_cc.parity
                         and abs(oj.tz2+ol.tz2)==2*tbc_ket_cc.Tz   and abs(ok.tz2+oi.tz2)==2*tbc_bra_cc.Tz
                         and j3min<=twoJ_ket_cc and twoJ_ket_cc<=j3max           and j4min<=twoJ_bra_cc and twoJ_bra_cc<=j4max )
               {
                 need_it=true;
                 break;
               }


             }
           }
           if (need_it)
           {
//             lookup[{ch_bra_cc,ch_ket_cc}].push_back({ch_bra,ch_ket});
             bra_list.push_back(ch_bra);
             ket_list.push_back(ch_ket);
           }
         }
       }
       lookup.at({ch_bra_cc,ch_ket_cc})[0].assign( begin(bra_list),end(bra_list) );
       lookup.at({ch_bra_cc,ch_ket_cc})[1].assign( begin(ket_list),end(ket_list) );
     }
   }
   profiler.timer["CalculatePandyaLookup"] += omp_get_wtime() - t_start;
   cout << "done." << endl;
}



