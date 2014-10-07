
#include "Operator.hh"
#include <cmath>
#include <iostream>
#include <iomanip>

using namespace std;

TwoBodyChannel::TwoBodyChannel()
{}

TwoBodyChannel::TwoBodyChannel(int j, int p, int t, Operator * op)
{
  Initialize(JMAX*2*t + JMAX*p + j, op);
}

TwoBodyChannel::TwoBodyChannel(int N, Operator * op)
{
   Initialize(N,op);
}

void TwoBodyChannel::Initialize(int N, Operator *op)
{
   J = N%JMAX;
   parity = (N/JMAX)%2;
   Tz = (N/(2*JMAX)-1);
   hermitian = op->IsHermitian();
   antihermitian = op->IsAntiHermitian();
   modelspace = op->GetModelSpace();
   NumberKets = 0;
   int nk = modelspace->GetNumberKets();
   KetMap.resize(nk,-1);
   for (int i=0;i<nk;i++)
   {
      Ket *ket = modelspace->GetKet(i);
      if ( CheckChannel_ket(ket) )
      {
         KetMap[i] = NumberKets;
         KetList.push_back(i);
         NumberKets++;
      }
   }
   TBME = arma::mat(NumberKets, NumberKets, arma::fill::zeros);
   Proj_pp = arma::mat(NumberKets, NumberKets, arma::fill::zeros);
   Proj_hh = arma::mat(NumberKets, NumberKets, arma::fill::zeros);
   //for (int &i: KetList) // C++11 syntax
   for (int i=0;i<NumberKets;i++) // C++11 syntax
   {
      Ket *ket = GetKet(i);
      if ( modelspace->GetOrbit(ket->p)->hvq ==0 and modelspace->GetOrbit(ket->q)->hvq==0)
      {
         Proj_hh(i,i) = 1;
      }
      if ( modelspace->GetOrbit(ket->p)->hvq >0 and modelspace->GetOrbit(ket->q)->hvq>0)
      {
         Proj_pp(i,i) = 1;
      }
   }

}


void TwoBodyChannel::Copy( const TwoBodyChannel& rhs)
{
   J                 = rhs.J;
   parity            = rhs.parity;
   Tz                = rhs.Tz;
   hermitian         = rhs.hermitian;
   antihermitian     = rhs.antihermitian;
   modelspace        = rhs.modelspace;
   NumberKets        = rhs.NumberKets;
   TBME              = rhs.TBME;
   Proj_hh           = rhs.Proj_hh;
   Proj_pp           = rhs.Proj_pp;
   KetMap            = rhs.KetMap;
   KetList           = rhs.KetList;
}


TwoBodyChannel& TwoBodyChannel::operator+=(const TwoBodyChannel& rhs)
{
   if (J == rhs.J and parity == rhs.parity and Tz == rhs.Tz)
   {
      hermitian         = (hermitian and rhs.hermitian);
      antihermitian     = (antihermitian and rhs.antihermitian);
      TBME              += rhs.TBME;
   }
   return *this;
}

TwoBodyChannel& TwoBodyChannel::operator-=(const TwoBodyChannel& rhs)
{
   if (J == rhs.J and parity == rhs.parity and Tz == rhs.Tz)
   {
      hermitian         = (hermitian and rhs.hermitian);
      antihermitian     = (antihermitian and rhs.antihermitian);
      TBME              -= rhs.TBME;
   }
   return *this;
}


double TwoBodyChannel::GetTBME(int bra, int ket) const 
{
   int bra_ind = GetLocalIndex(bra);
   int ket_ind = GetLocalIndex(ket);
   if (bra_ind < 0 or ket_ind < 0) return 0;
   if (bra_ind > NumberKets or ket_ind > NumberKets) return 0;
   return TBME(bra_ind, ket_ind);
}

double TwoBodyChannel::GetTBME(Ket *bra, Ket *ket) const
{
   int bra_ind = GetLocalIndex(bra->p,bra->q);
   int ket_ind = GetLocalIndex(ket->p,ket->q);
   //if (bra_ind < 0 or ket_ind < 0) return 1;
   if (bra_ind < 0 or ket_ind < 0)
   {
//      cout << "... No dice. bra_ind = " << bra_ind << " ket_ind = " << ket_ind << endl;
     return 0;
   }
   if (bra_ind > NumberKets or ket_ind > NumberKets) return 2;
   return TBME(bra_ind, ket_ind);
}

double TwoBodyChannel::GetTBME(int a, int b, int c, int d)
{
   cout << "Getting TBME " << a << " " << b << " " << c << " "<< d << endl;
   int ind_ab = GetLocalIndex(min(a,b),max(a,b));
   int ind_cd = GetLocalIndex(min(c,d),max(c,d));
   if (ind_ab < 0 or ind_cd < 0) return 0.0;
   double phase = 1.0;
   if (b<a) phase *= GetKet(ind_ab)->Phase(J);
   if (c<d) phase *= GetKet(ind_cd)->Phase(J);
   cout << "ind_ab = " << ind_ab << "  ind_cd = " << ind_cd << endl;
   return phase * TBME(ind_ab,ind_cd);
}

void TwoBodyChannel::SetTBME(int bra, int ket, float tbme)
{
   int bra_ind = GetLocalIndex(bra);
   int ket_ind = GetLocalIndex(ket);
   if ( (bra_ind < 0) or (ket_ind < 0) or
       (bra_ind > NumberKets) or (ket_ind > NumberKets) )
   {
     cout << "Bad assignment of tbme!"
         << " < " << bra << " | V | "
         << ket << " > "
         << "J,parity,Tz = " << J << "," << parity << "," << Tz
         << endl;
     return;
   }
   TBME(bra_ind, ket_ind) = tbme;
}

void TwoBodyChannel::SetTBME(Ket *bra, Ket *ket, float tbme)
{
   int bra_ind = GetLocalIndex(bra->p,bra->q);
   int ket_ind = GetLocalIndex(ket->p,ket->q);
   if ( (bra_ind < 0) or (ket_ind < 0) or
       (bra_ind > NumberKets) or (ket_ind > NumberKets) )
   {
      int ja,la,ta,jb,lb,tb;
      int a = bra->p;
      int b = bra->q;
      ja = modelspace->GetOrbit(a)->j2;
      la = modelspace->GetOrbit(a)->l;
      ta = modelspace->GetOrbit(a)->tz2;
      jb = modelspace->GetOrbit(b)->j2;
      lb = modelspace->GetOrbit(b)->l;
      tb = modelspace->GetOrbit(b)->tz2;
      cout << "Bad assignment of tbme!"
         << " < " << bra->p << " " << bra->q << " | V | "
         << ket->p << " " << ket->q << " > "
         << "J,parity,Tz = " << J << "," << parity << "," << Tz
         << "  bra_ind = " << bra_ind << "  ket_ind = " << ket_ind
         << endl
         << "    modelspace ket index = " << modelspace->GetKetIndex(ket)
         << "    modelspace bra index = " << modelspace->GetKetIndex(bra)
         << " out of " << modelspace->GetNumberKets()
         << endl
         << "  check_ket_channel = " << CheckChannel_ket(bra) << endl
         << " check parity: " << (la+lb)%2 << " = " << parity << "?" << endl
         << " check Tz: " << ta+tb << " = " << 2*Tz << "?" << endl
         << " check J: " << ja+jb << " >= " << 2*J << "?" << endl
         << " check J: " << abs(ja-jb) << " <= " << 2*J << "?" << endl

         << endl;
     return;
   }
   TBME(bra_ind, ket_ind) = tbme;
   //if (op->IsHermitian() )
   if ( hermitian )
      TBME(ket_ind, bra_ind) = tbme; 
   //if (op->IsAntiHermitian() )
   if ( antihermitian )
      TBME(ket_ind, bra_ind) = -tbme; 
}


bool TwoBodyChannel::CheckChannel_ket(int p, int q) const
{
   if ((p==q) and (J%2 != 0)) return false; // Pauli principle
   Orbit * op = modelspace->GetOrbit(p);
   Orbit * oq = modelspace->GetOrbit(q);
   if ((op->l + oq->l)%2 != parity) return false;
   if ((op->tz2 + oq->tz2) != 2*Tz) return false;
   if (op->j2 + oq->j2 < 2*J)       return false;
   if (abs(op->j2 - oq->j2) > 2*J)  return false;

   return true;
}

//===================================================================================
//===================================================================================
//  START IMPLEMENTATION OF OPERATOR METHODS
//===================================================================================
//===================================================================================

Operator::Operator()
{}

Operator::Operator(ModelSpace* ms) // Create a zero-valued operator in a given model space
{
  modelspace = ms;
  cout << "In Operator the modelspace has norbits = " << modelspace->GetNumberOrbits() << endl;
  hermitian = true;
  antihermitian = false;
  ZeroBody = 0;
  int nOneBody = modelspace->GetNumberOrbits();
  int nKets = modelspace->GetNumberKets();
  cout << "OneBody size = " << nOneBody*nOneBody << endl;
  cout << "TwoBody size = " << nKets*(nKets+1)/2 << endl;
  OneBody = arma::mat(nOneBody,nOneBody,arma::fill::zeros);
  TwoBodyJmax = 0;
  nChannels = 2*3*JMAX;

  for (int i=0;i<nChannels;++i)
  {
         SetTwoBodyChannel(i, TwoBodyChannel(i,this) );
         int J = GetTwoBodyChannel(i)->J;
         if (J > TwoBodyJmax and GetTwoBodyChannel(i)->GetNumberKets() > 0 ) TwoBodyJmax = J;
  }

  for (int i=0;i<nOneBody;i++)
  {
     OneBody(i,i) = modelspace->GetOrbit(i)->spe;
  }

  P_hole_onebody = arma::mat(nOneBody,nOneBody,arma::fill::zeros);
  P_particle_onebody = arma::mat(nOneBody,nOneBody,arma::fill::eye);
  for (int i=0;i<nOneBody;i++)
  {
     if (modelspace->GetOrbit(i)->hvq == 0)
     {
        P_hole_onebody(i,i) = 1;
        P_particle_onebody(i,i) = 0;
     }
  }
}

void Operator::Copy(const Operator& op)
{
   modelspace = op.modelspace;
   hermitian = op.hermitian;
   antihermitian = op.antihermitian;
   ZeroBody = op.ZeroBody;
   OneBody = op.OneBody;
   TwoBodyJmax = op.TwoBodyJmax;
   nChannels = op.nChannels;
   P_hole_onebody = op.P_hole_onebody;
   P_particle_onebody = op.P_particle_onebody;
   for (int ch=0;ch<nChannels;++ch)
   {
      SetTwoBodyChannel(ch, TwoBodyChannel(ch,this));
      GetTwoBodyChannel(ch)->SetTBME(op.GetTBME(ch));
   }
}


void Operator::PrintOut() 
{
   //for (int j=0; j<JMAX; j++)
   for (int j=0; j<TwoBodyJmax; j++)
   {
     for (int p=0; p<=1; p++)
     {
        for (int t=-1; t<=1; t++)
        {
           TwoBodyChannel *tbc = GetTwoBodyChannel(j,p,t);
//           cout << tbc.J << " " << tbc.parity << " "
//                     << tbc.Tz << "  ===> " << tbc.GetNumberKets() << endl;
           cout << tbc->J << " " << tbc->parity << " "
                     << tbc->Tz << "  ===> " << tbc->GetNumberKets() << endl;
           if (tbc->GetNumberKets()>20) continue;
          // if (tbc.GetNumberKets()>20) continue;
           for (int i=0;i<tbc->GetNumberKets();i++)
           //for (int i=0;i<tbc.GetNumberKets();i++)
           {
             //for (int ii=0;ii<tbc.GetNumberKets();ii++)
             for (int ii=0;ii<tbc->GetNumberKets();ii++)
             {
                //cout << tbc.TBME(i,ii) << " ";
                cout << tbc->TBME(i,ii) << " ";
             }
                cout << endl;
           }
        }
     }
   }
}




Operator Operator::DoNormalOrdering()
{
   // NOTE STILL NEED TO IMPLEMENT THE EFFECT OF THE {aaaa}, ie some terms go to zero
   Operator opNO = Operator(*this);
   opNO.ZeroBody = ZeroBody;
   opNO.OneBody = OneBody;
   int norbits = modelspace->GetNumberOrbits();

   for (int k=0;k<norbits;++k)
   {
      Orbit * orb = modelspace->GetOrbit(k);
      if (orb->hvq > 0) continue;
      opNO.ZeroBody += (orb->j2+1) * OneBody(k,k);
   }


   //////////////////////// v--This works, but it's the slow way (optimize if needed) /////////////////////
   for (int ch=0;ch<nChannels;++ch)
   {
      TwoBodyChannel *tbc = GetTwoBodyChannel(ch);
      opNO.SetTwoBodyChannel(ch, *tbc );
      int npq = tbc->GetNumberKets();
      int J = tbc->J;

      int ibra,iket;
      for (int a=0;a<norbits;++a)
      {
         Orbit *orba = modelspace->GetOrbit(a);
         for (int b=a;b<norbits;++b)
         {
            Orbit *orbb = modelspace->GetOrbit(b);
            if (orbb->j2 != orba->j2 or orbb->tz2 != orba->tz2 or orbb->l != orba->l) continue;
            //for (int c=0;c<norbits;++c)
            for (int &h : modelspace->hole)  // C++11 syntax
            {
              // if ( modelspace->GetOrbit(c)->hvq >0 ) continue;
               if ( (ibra = tbc->GetLocalIndex(min(a,h),max(a,h)))<0) continue;
               if ( (iket = tbc->GetLocalIndex(min(b,h),max(b,h)))<0) continue;
               Ket * bra = tbc->GetKet(ibra);
               Ket * ket = tbc->GetKet(iket);
               double tbme = (2*J+1.0)/(orba->j2+1)*tbc->GetTBME(bra,ket);
               if (a<=h and b<=h) opNO.OneBody(a,b) += tbme;
               if (a>h and b<=h) opNO.OneBody(a,b) += bra->Phase(J)*tbme;
               if (a<=h and b>h) opNO.OneBody(a,b) += ket->Phase(J)*tbme;
               if (a>h and b>h) opNO.OneBody(a,b) += bra->Phase(J)*ket->Phase(J)*tbme;
            }
            if (hermitian) opNO.OneBody(b,a) = opNO.OneBody(a,b);
            if (antihermitian) opNO.OneBody(b,a) = -opNO.OneBody(a,b);
         }
      }
   //////////////////////// ^-- This works, but it's the slow way (I think) /////////////////////


      for (int k=0;k<npq;++k) // loop over kets in this channel
      {
         Ket * ket = tbc->GetKet(k);
         Orbit *oa = modelspace->GetOrbit(ket->p);
         Orbit *ob = modelspace->GetOrbit(ket->q);

         if (oa->hvq > 0 and ob->hvq > 0) continue; // need at least one orbit in the core

         // Sum over diagonal two-body terms with both orbits in the core
         if ( oa->hvq == 0 and ob->hvq == 0)
         {
            opNO.ZeroBody += tbc->TBME(k,k) * (2*tbc->J+1) / (1.0+ket->delta_pq());  // <ab|V|ab>  (a,b in core)
         }

      } // for k
   } // for ch
   return opNO;
}



void Operator::EraseTwoBody()
{
   for (int ch=0;ch<nChannels;++ch)
   {
       GetTwoBodyChannel(ch)->TBME.zeros();
   }
}



//Operator Operator::Commutator(const Operator& opright)
Operator Operator::Commutator(Operator& opright)
{
   Operator out = opright;
   out.EraseZeroBody();
   out.EraseOneBody();
   out.EraseTwoBody();

//   out.ZeroBody  = comm110(OneBody, opright.OneBody);
   cout << "Zero Body" << endl;
   out.ZeroBody  = comm110(opright) + comm220(opright) ;
//   out.ZeroBody += comm220(opright);

   cout << "One Body" << endl;
//   out.OneBody  = comm111(OneBody, opright.OneBody);
   out.OneBody  = comm111(opright) + comm211(opright) -opright.comm211(*this)  + comm221(opright);

   cout << "Two Body" << endl;
   for (int ch=0;ch<nChannels; ++ch)
   {
      out.TwoBody[ch] = comm112(opright,ch) + comm212(opright,ch) - opright.comm212(*this,ch) + comm222(opright,ch);
   }
/*
   for (int ch=0;ch<nChannels;++ch)
   {
       if (TwoBody[ch].J > TwoBodyJmax) continue;
//            out.ZeroBody += comm220(TwoBody[ch], opright.TwoBody[ch]);

            out.OneBody += comm211(TwoBody[ch], opright.OneBody) - comm211(opright.TwoBody[ch],OneBody);
            out.OneBody += comm221(TwoBody[ch], opright.TwoBody[ch]);

            out.TwoBody[ch]  = comm112(OneBody, opright.OneBody);
            out.TwoBody[ch] += comm212(TwoBody[ch], opright.OneBody) - comm212(opright.TwoBody[ch],OneBody);
            out.TwoBody[ch] += comm222(TwoBody[ch], opright.TwoBody[ch]);


   }
*/
   return out;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// Below is the implementation of the commutators in the various channels
///////////////////////////////////////////////////////////////////////////////////////////

//*****************************************************************************************
//                ____Y    __         ____X
//          X ___(_)             Y___(_) 
//
//  [ X^(1), Y^(1) ]^(0) = Sum_ab (2j_a+1) x_ab y_ab  (n_a-n_b) <-- this is in Koshiroh's appendix, and I think it's wrong.
//  [ X^(1), Y^(1) ]^(0) = Sum_ab (2j_a+1) x_ab y_ba  (n_a-n_b) <-- I think this is right.
//                       = Sum_a  (2j_a+1)  (xy-yx)_aa n_a
//
double Operator::comm110(Operator& opright)
{
   double comm = 0;
   int norbits = modelspace->GetNumberOrbits();
   arma::mat xyyx = OneBody*opright.OneBody - opright.OneBody*OneBody;
   for ( int& a : modelspace->particle) // C++11 range-for syntax: loop over all elements of the vector <particle>
   {
      comm += (modelspace->GetOrbit(a)->j2+1)*xyyx(a,a);
   }
   return comm;
}


//*****************************************************************************************
//         __Y__       __X__
//        ()_ _()  -  ()_ _()
//           X           Y
//
//  [ X^(2), Y^(2) ]^(0) = 1/2 Sum_abcd  Sum_J (2J+1) x_abcd y_cdab (n_a n_b nbar_c nbar_d)
//                       = 1/2 Sum_J (2J+1) Sum_abcd x_abcd y_cdab (n_a n_b nbar_c nbar_d)  
//
//
double Operator::comm220( Operator& opright)
{
//  cout << "In comm220" << endl;
   double comm = 0;
   for (int ch=0;ch<nChannels;++ch)
   {
      TwoBodyChannel *tbleft = GetTwoBodyChannel(ch);
      TwoBodyChannel *tbright = opright.GetTwoBodyChannel(ch);
      comm += 0.5*(2*tbleft->J+1) * arma::accu( tbleft->Proj_hh * tbleft->TBME * tbleft->Proj_pp * tbright->TBME);
   }
   return comm;
}

//*****************************************************************************************
//
//        |____. Y          |___.X
//        |        _        |
//  X .___|            Y.___|              [X^(1),Y^(1)]^(1)  =  XY - YX
//        |                 |

//arma::mat Operator::comm111(const arma::mat& obleft, const arma::mat& obright)
arma::mat Operator::comm111(Operator & opright)
{
//  cout << "In comm111" << endl;
//   arma::mat comm = obleft*obright - obright*obleft;
//   return comm;
   return OneBody*opright.OneBody - opright.OneBody*OneBody;
}

//*****************************************************************************************
// 
//      i |              i |
//        |    ___.Y       |__X__
//        |___(_)    _     |   (_)__.     [X^(2),Y^(1)]^(1)  =  1/(2j_i+1) sum_ab(n_a-n_b)y_ab 
//      j | X            j |        Y            * sum_J (2J+1) x_biaj^(J)  
//                                             
//                                               = 1/(2j+1) sum_a n_a sum_J (2J+1)
//                                                  * sum_b y_ab x_biaj - yba x_aibj
//
arma::mat Operator::comm211(Operator& opright)
{
//  cout << "In comm211" << endl;
   int norbits = modelspace->GetNumberOrbits();
   arma::mat comm = arma::mat(norbits,norbits,arma::fill::zeros);
   for (int i=0;i<norbits;++i)
   {
      Orbit *oi = modelspace->GetOrbit(i);
      for (int j=0;j<norbits; ++j) // Later make this j=i;j<norbits... and worry about Hermitian vs anti-Hermitian
      {
          Orbit *oj = modelspace->GetOrbit(j);
          if (oi->j2 != oj->j2 or oi->l != oj->l or oi->tz2 != oj->tz2) continue; // At some point, make a OneBodyChannel class...
          double commij = 0;
          for (int &a : modelspace->hole)  // C++11 syntax
          {
             for (int ch=0;ch<nChannels;++ch)
             {
                TwoBodyChannel *tbc = GetTwoBodyChannel(ch);
                if (tbc->GetNumberKets() < 1) continue;
                double commijJ = 0;
                int ind_ai = tbc->GetLocalIndex(min(a,i),max(a,i));
                int ind_aj = tbc->GetLocalIndex(min(a,j),max(a,j));
                if ( ind_ai < 0 and ind_aj < 0) continue; 
                for (int b=0;b<norbits; ++b)
                {
                   commijJ += opright.OneBody(a,b) * tbc->GetTBME(b,i,a,j) - opright.OneBody(b,a) * tbc->GetTBME(a,i,b,j);
                }
                comm(i,j) += commijJ*(2*tbc->J+1);
             }
          }
          comm(i,j) /= (modelspace->GetOrbit(i)->j2 + 1);
      }
   }
   return comm;
}

//*****************************************************************************************
//
//      i |             i |
//        |__Y___         |__X___
//        |____(_)  _     |____(_)     [X^(2),Y^(2)]^(1)  =  1/(2(2j_i+1)) sum_J (2J+1) 
//      j | X           j |  Y            * sum_abc (nbar_a*nbar_b*n_c + n_a*n_b*nbar_c)
//                                         * (x_ciab y_abcj - y_ciab xabcj)
// NOT YET FINISHED...
arma::mat Operator::comm221(Operator& opright)
{

   arma::mat comm = arma::mat(modelspace->GetNumberOrbits(),modelspace->GetNumberOrbits(),arma::fill::zeros);
   for (int ch=0;ch<nChannels;++ch)
   {
      TwoBodyChannel *tbleft = GetTwoBodyChannel(ch);
      TwoBodyChannel *tbright = opright.GetTwoBodyChannel(ch);
   //  cout << "In comm221" << endl;
      int npq = tbleft->GetNumberKets();
      // Projector onto hole states, etc
      arma::mat P_h = arma::mat(npq,npq,arma::fill::zeros);
      arma::mat P_p = arma::mat(npq,npq,arma::fill::eye);
      for(int i=0;i<npq;i++)
      {
         int ketindex = tbleft->GetKetIndex(i);
         int a = modelspace->GetKet(ketindex)->p;
         int b = modelspace->GetKet(ketindex)->q;
         if (modelspace->GetOrbit(a)->hvq == 0 )
         { 
           P_h(i,i)=1;
           P_p(i,i)=0;
         }
      }
   
      arma::mat Mpph = P_h*(tbleft->TBME * tbleft->Proj_pp * tbright->TBME - tbright->TBME * tbleft->Proj_pp * tbleft->TBME);
      arma::mat Mhhp = P_p * (tbleft->TBME * tbleft->Proj_hh * tbright->TBME - tbright->TBME * tbleft->Proj_hh * tbleft->TBME);

      for (int ibra=0;ibra<npq;ibra++)
      {
         for (int iket=0;iket<npq;iket++)
         {
            
         }
      }
   }
   
   return comm;
}

//*****************************************************************************************

TwoBodyChannel Operator::comm112(Operator& opright, int ch)
{
//  cout << "In comm122" << endl;
   TwoBodyChannel comm = TwoBody[ch];
   return comm;
}

//*****************************************************************************************

TwoBodyChannel Operator::comm212(Operator& opright, int ch )
{
//  cout << "In comm212" << endl;
   TwoBodyChannel comm = TwoBody[ch];
   return comm;
}

//*****************************************************************************************

TwoBodyChannel Operator::comm222(Operator& opright, int ch )
{
//  cout << "In comm222" << endl;
   TwoBodyChannel comm = TwoBody[ch];
   return comm;
}

//*****************************************************************************************


