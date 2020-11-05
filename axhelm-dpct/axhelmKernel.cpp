#include <CL/sycl.hpp>
#include <dpct/dpct.hpp>

SYCL_EXTERNAL void axhelm(const int Nelements, const int offset,
                          const double *__restrict__ ggeo,
                          const double *__restrict__ D,
                          const double *__restrict__ lambda,
                          const double *__restrict__ q, double *__restrict__ Aq,
                          sycl::nd_item<3> item_ct1, double *s_D, double *s_q,
                          double *s_Gqr, double *s_Gqs)
{

  double r_qt, r_Gqt, r_Auk;
  double r_q[8];
  double r_Aq[8];
  double r_G00, r_G01, r_G02, r_G11, r_G12, r_G22, r_GwJ;
  double r_lam0, r_lam1;

  int e = item_ct1.get_group(2);
  int j = item_ct1.get_local_id(1);
  int i = item_ct1.get_local_id(2);
  s_D[j*8+i] = D[j*8+i];
  const int base = i + j * 8 + e * 512;
  for (int k = 0; k < 8; ++k) {
    r_q[k] = q[base + k * 8 * 8];
    r_Aq[k] = 0;
  }
#pragma unroll 8
  for (int k = 0; k < 8; ++k) {
    const int id = e * 512 + k * 8 * 8 + j * 8 + i;
    const int gbase = e * p_Nggeo * 512 + k * 8 * 8 + j * 8 + i;
    r_G00 = ggeo[gbase + p_G00ID * 512];
    r_G01 = ggeo[gbase + p_G01ID * 512];
    r_G02 = ggeo[gbase + p_G02ID * 512];
    r_G11 = ggeo[gbase + p_G11ID * 512];
    r_G12 = ggeo[gbase + p_G12ID * 512];
    r_G22 = ggeo[gbase + p_G22ID * 512];
    r_GwJ = ggeo[gbase + p_GWJID * 512];
    r_lam0 = lambda[id + 0 * offset];
    r_lam1 = lambda[id + 1 * offset];
    item_ct1.barrier();
    s_q[j*8+i] = r_q[k];
    r_qt = 0;
#pragma unroll 8
    for (int m = 0; m < 8; ++m) {
      r_qt += s_D[k*8+m] * r_q[m];
    }
    item_ct1.barrier();
    double qr = 0;
    double qs = 0;
#pragma unroll 8
    for (int m = 0; m < 8; ++m) {
      qr += s_D[i*8+m] * s_q[j*8+m];
      qs += s_D[j*8+m] * s_q[m*8+i];
    }
    s_Gqs[j*8+i] = r_lam0 * (r_G01 * qr + r_G11 * qs + r_G12 * r_qt);
    s_Gqr[j*8+i] = r_lam0 * (r_G00 * qr + r_G01 * qs + r_G02 * r_qt);
    r_Gqt = r_lam0 * (r_G02 * qr + r_G12 * qs + r_G22 * r_qt);
    r_Auk = r_GwJ * r_lam1 * r_q[k];
    item_ct1.barrier();
#pragma unroll 8
    for (int m = 0; m < 8; ++m) {
      r_Auk += s_D[m*8+j] * s_Gqs[m*8+i];
      r_Aq[m] += s_D[k*8+m] * r_Gqt;
      r_Auk += s_D[m*8+i] * s_Gqr[j*8+m];
    }
    r_Aq[k] += r_Auk;
    item_ct1.barrier();
  }
#pragma unroll 8
  for (int k = 0; k < 8; ++k) {
    const int id = e * 512 + k * 8 * 8 + j * 8 + i;
    Aq[id] = r_Aq[k];
  }
}

SYCL_EXTERNAL void
axhelm_n3(const int Nelements, const int offset,
          const double *__restrict__ ggeo, const double *__restrict__ D,
          const double *__restrict__ lambda, const double *__restrict__ q,
          double *__restrict__ Aq, sycl::nd_item<3> item_ct1, double *s_D,
          double *s_U, double *s_V, double *s_W, double *s_GUr, double *s_GUs,
          double *s_GVr, double *s_GVs, double *s_GWr, double *s_GWs)
{

  double r_Ut, r_Vt, r_Wt;
  double r_U[8], r_V[8], r_W[8];
  double r_AU[8], r_AV[8], r_AW[8];
  double r_G00, r_G01, r_G02, r_G11, r_G12, r_G22, r_GwJ;
  double r_lam0, r_lam1;

  int e = item_ct1.get_group(2);
  int j = item_ct1.get_local_id(1);
  int i = item_ct1.get_local_id(2);
  s_D[j*8+i] = D[j*8+i];
  const int base = i + j * 8 + e * 512;
  for (int k = 0; k < 8; k++) {
    r_U[k] = q[base + k * 8 * 8 + 0 * offset];
    r_V[k] = q[base + k * 8 * 8 + 1 * offset];
    r_W[k] = q[base + k * 8 * 8 + 2 * offset];
    r_AU[k] = 0;
    r_AV[k] = 0;
    r_AW[k] = 0;
  }
#pragma unroll 8
  for (int k = 0; k < 8; ++k) {
    const int id = e * 512 + k * 8 * 8 + j * 8 + i;
    const int gbase = e * p_Nggeo * 512 + k * 8 * 8 + j * 8 + i;
    r_G00 = ggeo[gbase + p_G00ID * 512];
    r_G01 = ggeo[gbase + p_G01ID * 512];
    r_G02 = ggeo[gbase + p_G02ID * 512];
    r_G11 = ggeo[gbase + p_G11ID * 512];
    r_G12 = ggeo[gbase + p_G12ID * 512];
    r_G22 = ggeo[gbase + p_G22ID * 512];
    r_GwJ = ggeo[gbase + p_GWJID * 512];
    r_lam0 = lambda[id + 0 * offset];
    r_lam1 = lambda[id + 1 * offset];
    item_ct1.barrier();
    s_U[j*8+i] = r_U[k];
    s_V[j*8+i] = r_V[k];
    s_W[j*8+i] = r_W[k];
    r_Ut = 0;
    r_Vt = 0;
    r_Wt = 0;
#pragma unroll 8
    for (int m = 0; m < 8; m++) {
      double Dkm = s_D[k*8+m];
      r_Ut += Dkm * r_U[m];
      r_Vt += Dkm * r_V[m];
      r_Wt += Dkm * r_W[m];
    }
    item_ct1.barrier();
    double Ur = 0, Us = 0;
    double Vr = 0, Vs = 0;
    double Wr = 0, Ws = 0;
#pragma unroll 8
    for (int m = 0; m < 8; m++) {
      double Dim = s_D[i*8+m];
      double Djm = s_D[j*8+m];
      Ur += Dim * s_U[j*8+m];
      Us += Djm * s_U[m*8+i];
      Vr += Dim * s_V[j*8+m];
      Vs += Djm * s_V[m*8+i];
      Wr += Dim * s_W[j*8+m];
      Ws += Djm * s_W[m*8+i];
    }
    s_GUr[j*8+i] = r_lam0 * (r_G00 * Ur + r_G01 * Us + r_G02 * r_Ut);
    s_GVr[j*8+i] = r_lam0 * (r_G00 * Vr + r_G01 * Vs + r_G02 * r_Vt);
    s_GWr[j*8+i] = r_lam0 * (r_G00 * Wr + r_G01 * Ws + r_G02 * r_Wt);
    s_GUs[j*8+i] = r_lam0 * (r_G01 * Ur + r_G11 * Us + r_G12 * r_Ut);
    s_GVs[j*8+i] = r_lam0 * (r_G01 * Vr + r_G11 * Vs + r_G12 * r_Vt);
    s_GWs[j*8+i] = r_lam0 * (r_G01 * Wr + r_G11 * Ws + r_G12 * r_Wt);
    r_Ut = r_lam0 * (r_G02 * Ur + r_G12 * Us + r_G22 * r_Ut);
    r_Vt = r_lam0 * (r_G02 * Vr + r_G12 * Vs + r_G22 * r_Vt);
    r_Wt = r_lam0 * (r_G02 * Wr + r_G12 * Ws + r_G22 * r_Wt);
    r_AU[k] += r_GwJ * r_lam1 * r_U[k];
    r_AV[k] += r_GwJ * r_lam1 * r_V[k];
    r_AW[k] += r_GwJ * r_lam1 * r_W[k];
    item_ct1.barrier();
    double AUtmp = 0, AVtmp = 0, AWtmp = 0;
#pragma unroll 8
    for (int m = 0; m < 8; m++) {
      double Dmi = s_D[m*8+i];
      double Dmj = s_D[m*8+j];
      double Dkm = s_D[k*8+m];
      AUtmp += Dmi * s_GUr[j*8+m];
      AUtmp += Dmj * s_GUs[m*8+i];
      AVtmp += Dmi * s_GVr[j*8+m];
      AVtmp += Dmj * s_GVs[m*8+i];
      AWtmp += Dmi * s_GWr[j*8+m];
      AWtmp += Dmj * s_GWs[m*8+i];
      r_AU[m] += Dkm * r_Ut;
      r_AV[m] += Dkm * r_Vt;
      r_AW[m] += Dkm * r_Wt;
    }
    r_AU[k] += AUtmp;
    r_AV[k] += AVtmp;
    r_AW[k] += AWtmp;
  }
#pragma unroll 8
  for (int k = 0; k < 8; k++) {
    const int id = e * 512 + k * 8 * 8 + j * 8 + i;
    Aq[id + 0 * offset] = r_AU[k];
    Aq[id + 1 * offset] = r_AV[k];
    Aq[id + 2 * offset] = r_AW[k];
  }
}
