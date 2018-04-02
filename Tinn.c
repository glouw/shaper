#include "Tinn.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Error function.
static float err(float a, float b)
{
    return 0.5f * powf(a - b, 2.0f);
}

// Partial derivative of error function.
static float pderr(float a, float b)
{
    return a - b;
}

// Total error.
static float terr(const float* tg, const float* o, int size)
{
    float sum = 0.0f;
    for(int i = 0; i < size; i++)
        sum += err(tg[i], o[i]);
    return sum;
}

// Activation function.
static float act(float a)
{
    return 1.0f / (1.0f + expf(-a));
}

// Partial derivative of activation function.
static float pdact(float a)
{
    return a * (1.0f - a);
}

// Floating point random from 0.0 - 1.0.
static float frand()
{
    return rand() / (float) RAND_MAX;
}

// Back propagation.
static void backwards(const Tinn t, const float* in, const float* tg, float rate)
{
    for(int i = 0; i < t.nhid; i++)
    {
        float sum = 0.0f;
        // Calculate total error change with respect to output.
        for(int j = 0; j < t.nops; j++)
        {
            const float a = pderr(t.o[j], tg[j]);
            const float b = pdact(t.o[j]);
            sum += a * b * t.x[j * t.nhid + i];
            // Correct weights in hidden to output layer.
            t.x[j * t.nhid + i] -= rate * a * b * t.h[i];
        }
        // Correct weights in input to hidden layer.
        for(int j = 0; j < t.nips; j++)
            t.w[i * t.nips + j] -= rate * sum * pdact(t.h[i]) * in[j];
    }
}

// Forward propagation.
static void forwards(const Tinn t, const double* in)
{
    // Calculate hidden layer neuron values.
    for(int i = 0; i < t.nhid; i++)
    {
        float sum = 0.0f;
        for(int j = 0; j < t.nips; j++)
            sum += in[j] * t.w[i * t.nips + j];
        t.h[i] = act(sum + t.b[0]);
    }
    // Calculate output layer neuron values.
    for(int i = 0; i < t.nops; i++)
    {
        float sum = 0.0f;
        for(int j = 0; j < t.nhid; j++)
            sum += t.h[j] * t.x[i * t.nhid + j];
        t.o[i] = act(sum + t.b[1]);
    }
}

// Randomizes weights and biases.
static void twrand(const Tinn t)
{
    for(int i = 0; i < t.nw; i++) t.w[i] = frand() - 0.5f;
    for(int i = 0; i < t.nb; i++) t.b[i] = frand() - 0.5f;
}

// Prints a message and exits.
static void bomb(const char* const message, ...)
{
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    exit(1);
}

// Fail safe file opening.
static FILE* efopen(const char* const pathname, const char* const mode)
{
    FILE* const file = fopen(pathname, mode);
    if(file == NULL)
        bomb("failure: fopen(\"%s\", \"%s\")\n", pathname, mode);
    return file;
}

// Fail safe clear allocation.
static void* ecalloc(const size_t nmemb, const size_t size)
{
    void* const mem = calloc(nmemb, size);
    if(mem == NULL)
        bomb("failure: calloc(%d, %d)\n", nmemb, size);
    return mem;
}

float* xpredict(const Tinn t, const float* in)
{
    forwards(t, in);
    return t.o;
}

float xttrain(const Tinn t, const float* in, const float* tg, float rate)
{
    forwards(t, in);
    backwards(t, in, tg, rate);
    return terr(tg, t.o, t.nops);
}

Tinn xtbuild(int nips, int nhid, int nops)
{
    Tinn t;
    // Tinn only supports one hidden layer so there are two biases.
    t.nb = 2;
    t.nw = nhid * (nips + nops);
    t.w = (float*) ecalloc(t.nw, sizeof(*t.w));
    t.x = t.w + nhid * nips;
    t.b = (float*) ecalloc(t.nb, sizeof(*t.b));
    t.h = (float*) ecalloc(nhid, sizeof(*t.h));
    t.o = (float*) ecalloc(nops, sizeof(*t.o));
    t.nips = nips;
    t.nhid = nhid;
    t.nops = nops;
    twrand(t);
    return t;
}

void xtsave(const Tinn t, const char* path)
{
    FILE* const file = efopen(path, "w");
    // Header.
    fprintf(file, "%d %d %d\n", t.nips, t.nhid, t.nops);
    // Biases and weights.
    for(int i = 0; i < t.nb; i++) fprintf(file, "%f\n", (double) t.b[i]);
    for(int i = 0; i < t.nw; i++) fprintf(file, "%f\n", (double) t.w[i]);
    fclose(file);
}

Tinn xtload(const char* path)
{
    FILE* const file = efopen(path, "r");
    int nips = 0;
    int nhid = 0;
    int nops = 0;
    // Header.
    fscanf(file, "%d %d %d\n", &nips, &nhid, &nops);
    // A new tinn is returned.
    const Tinn t = xtbuild(nips, nhid, nips);
    // Biases and weights.
    for(int i = 0; i < t.nb; i++) fscanf(file, "%f\n", &t.b[i]);
    for(int i = 0; i < t.nw; i++) fscanf(file, "%f\n", &t.w[i]);
    fclose(file);
    return t;
}

void xtfree(const Tinn t)
{
    free(t.w);
    free(t.b);
    free(t.h);
    free(t.o);
}
