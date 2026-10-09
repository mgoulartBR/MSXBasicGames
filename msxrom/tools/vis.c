/* LOS 2D em lote: para cada consulta (x1,y1,x2,y2) diz se NENHUM bloqueador (segmento) a corta. */
#include <stdint.h>
static int cross(double ax,double ay,double bx,double by,double cx,double cy,double dx,double dy){
  double d1=(bx-ax)*(cy-ay)-(by-ay)*(cx-ax), d2=(bx-ax)*(dy-ay)-(by-ay)*(dx-ax);
  double d3=(dx-cx)*(ay-cy)-(dy-cy)*(ax-cx), d4=(dx-cx)*(by-cy)-(dy-cy)*(bx-cx);
  return ((d1>0&&d2<0)||(d1<0&&d2>0))&&((d3>0&&d4<0)||(d3<0&&d4>0));
}
void los_batch(int ns,const double *s,int nq,const double *q,uint8_t *out){
  for(int i=0;i<nq;i++){
    const double *p=q+4*i; int ok=1;
    for(int j=0;j<ns&&ok;j++){ const double *b=s+4*j; if(cross(p[0],p[1],p[2],p[3],b[0],b[1],b[2],b[3])) ok=0; }
    out[i]=ok;
  }
}
