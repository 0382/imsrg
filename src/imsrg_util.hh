///////////////////////////////////////////////////////////////////////////////////
//    imsrg_util.hh, part of  imsrg++
//    Copyright (C) 2018  Ragnar Stroberg
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
///////////////////////////////////////////////////////////////////////////////////

#ifndef imsrg_util_hh
#define imsrg_util_hh 1

#include "ModelSpace.hh"
#include "Operator.hh"
#include "HartreeFock.hh"
#include "IMSRGSolver.hh"
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf_laguerre.h>
#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_sf_bessel.h>
#include <vector>

#define HBARC 197.3269718 // hc in MeV * fm
#define M_NUCLEON 938.9185 // average nucleon mass in MeV
#define PI 3.14159265359 // put in by CP for the BMEs
#define M_PROTON  938.2720813
#define M_NEUTRON 939.5654133

#ifndef ISQRT2
  #define ISQRT2 0.70710678118654752440L
#endif

namespace imsrg_util
{
 Operator OperatorFromString(ModelSpace& modelspace, std::string str);
 std::map<index_t,double> GetSecondOrderOccupations(Operator& H, int emax);

 Operator NumberOp(ModelSpace& modelspace, int n, int l, int j2, int tz2);
 Operator NumberOpAlln(ModelSpace& modelspace, int l, int j2, int tz2);
 Operator PSquaredOp(ModelSpace& modelspace);
 Operator RSquaredOp(ModelSpace& modelspace);
 Operator E0Op(ModelSpace& modelspace);
 Operator ElectricMultipoleOp(ModelSpace& modelspace, int L);
 Operator NeutronElectricMultipoleOp(ModelSpace& modelspace, int L);
 Operator IntrinsicElectricMultipoleOp(ModelSpace& modelspace, int L);
 Operator MagneticMultipoleOp(ModelSpace& modelspace, int L);
 Operator MagneticMultipoleOp_pn(ModelSpace& modelspace, int L, std::string pn);
 Operator Trel_Op(ModelSpace& modelspace);
 Operator TCM_Op(ModelSpace& modelspace);
 Operator HCM_Op(ModelSpace& modelspace);
 Operator Trel_Masscorrection_Op(ModelSpace& modelspace);
 Operator KineticEnergy_Op(ModelSpace& modelspace);
 Operator KineticEnergy_RelativisticCorr(ModelSpace& modelspace);

 Operator R2CM_Op(ModelSpace& modelspace);
 Operator Rp2_corrected_Op(ModelSpace& modelspace, int A, int Z);
 Operator Rn2_corrected_Op(ModelSpace& modelspace, int A, int Z);
 Operator Rm2_corrected_Op(ModelSpace& modelspace, int A, int Z);
 Operator R2_p1_Op(ModelSpace& modelspace);
 Operator R2_1body_Op(ModelSpace& modelspace, std::string option);
 Operator R2_p2_Op(ModelSpace& modelspace);
 Operator R2_2body_Op(ModelSpace& modelspace, std::string option);
 Operator ProtonDensityAtR(ModelSpace& modelspace, double R);
 Operator NeutronDensityAtR(ModelSpace& modelspace, double R);
 Operator RpSpinOrbitCorrection(ModelSpace& modelspace);
 Operator FourierBesselCoeff(ModelSpace& modelspace, int nu, double R, std::vector<index_t> index_list);

 Operator Isospin2_Op(ModelSpace& modelspace);
 Operator AllowedFermi_Op(ModelSpace& modelspace);
 Operator AllowedGamowTeller_Op(ModelSpace& modelspace);
 Operator Sigma_Op(ModelSpace& modelspace);
 Operator Sigma_Op_pn(ModelSpace& modelspace, std::string pn);
 Operator RadialOverlap(ModelSpace& modelspace);
 Operator LdotS_Op(ModelSpace& modelspace);
 Operator L2rel_Op(ModelSpace& modelspace);
 Operator LCM_Op(ModelSpace& modelspace);
 Operator QdotQ_Op(ModelSpace& modelspace);

 Operator Dagger_Op( ModelSpace& modelspace, index_t Q );

 Operator Density_sog( double R_i, double gamma, std::string pn );

 Operator FirstOrderCorr_1b( const Operator& OpIn, const Operator& H );
// Operator RPA_resummed_1b( const Operator& OpIn, const Operator& H );
 Operator RPA_resummed_1b( const Operator& OpIn, const Operator& H, std::string mode="RPA" );
 arma::mat GetPH_transformed_Gamma( std::vector<std::pair<size_t,size_t>>& bras, std::vector<std::pair<size_t,size_t>>& kets, const Operator& H, int Lambda );


////////////////// Double beta decay functions from Charlie Payne ///////////////
 Operator M0nu_TBME_Op(ModelSpace& modelspace, int Nquad, std::string src); // put in by CP, it is still in development
 double CPrbmeGen(ModelSpace& modelspace, double rho, double x, int n, int l, int np, int lp, int mm, double pp); // testing...
 inline double cpNorm(int a, int b) {return a==b ? ISQRT2 : 1.0;}; // put in by CP, for anti-symmetrization normalization
// SRS - the following three don't seem to be used.
// double CPrbme(ModelSpace& modelspace, double x, int n, int l, int np); // put in by CP, function for M0nu_TBME_Op above
// Operator CPchecker_Op(ModelSpace& modelspace); // put in by CP
// Operator CharlieSdS_Op(ModelSpace& modelspace); // put in by CP
// Operator RagnarSdS_Op(ModelSpace& modelspace); // put in by CP

/////////////////////////////////////////////////////////////////////////////////


 Operator EKKShift( Operator& Hin, int Nlower, int Nupper);

 Operator MinnesotaPotential( ModelSpace& modelspace );
 double MinnesotaMatEl( ModelSpace& modelspace, Ket& bra, Ket& ket, int J );


 Operator Single_Ref_1B_Density_Matrix(ModelSpace& modelspace); // This doesn't work
 double Get_Charge_Density(Operator& DM, double r);  // This doesn't work


 double Calculate_p1p2(ModelSpace& modelspace, Ket & bra, Ket & ket, int J);
 void Calculate_p1p2_all(Operator& OpIn);
 double Calculate_r1r2(ModelSpace& modelspace, Ket & bra, Ket & ket, int J);
 double HO_density(int n, int l, double hw, double r);
 double HO_Radial_psi(int n, int l, double hw, double r);
 double RadialIntegral(int na, int la, int nb, int lb, int L);
 double RadialIntegral_RpowK(int na, int la, int nb, int lb, int k);
 double TalmiI(int p, double k);
 double TalmiB(int na, int la, int nb, int lb, int p);
 std::vector<double> GetOccupationsHF(HartreeFock& hf);
 std::vector<double> GetOccupations(HartreeFock& hf, IMSRGSolver& imsrgsolver);
 std::vector<double> GetDensity(std::vector<double>& occ, std::vector<double>& R, std::vector<int>& orbits, ModelSpace& modelspace);

 void WriteSPWaveFunctions( std::vector<std::string>& sporbits, HartreeFock& hf, std::string intfile );

 void Embed1BodyIn2Body(Operator& op1, int A);
 double GetEmbeddedTBME(Operator& op1, index_t i, index_t j, index_t k, index_t l, int Jbra,int Jket, int Lambda);

 double FrequencyConversionCoeff(int n1, int l1, double hw1, int n2, int l2, double hw2);

 void CommutatorTest(Operator& X, Operator& Y);
 void Reduce(Operator&);
 void UnReduce(Operator&);

 void SplitUp(Operator& OpIn, Operator& OpLow, Operator& OpHi, int ecut);


// Templated functions need to be defined in the header file (or else explicitly declared in the .cc file).
 template <typename T>
 T VectorUnion(const T& v1)
 {
   return v1;
 }
 
 template <typename T, typename... Args>
 T VectorUnion(const T& v1, const T& v2, Args... args)
 {
   T vec(v1.size()+v2.size());
   std::copy(v1.begin(),v1.end(),vec.begin());
   std::copy(v2.begin(),v2.end(),vec.begin()+v1.size());
   return VectorUnion(vec, args...);
 }

}




#endif
