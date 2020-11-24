#include <math.h>

int request(int min, int max) {

    /* k is log(MAX/MIN) */
    double k = log(((double) max ) / min);

    /* r is [0..k] */
    double r = ((double)( rand() % (int)(k*10000))) / 10000;

    /* size is [0 .. MAX] */
    int size = (int)((double)max / exp(r));

    return size;
}