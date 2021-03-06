//
//  QYTSampler.cpp
//  qyt_render_ver1.5
//
//  Created by nocolor on 15/3/1.
//  Copyright (c) 2015年 ___JinJiangTiaoZhanBei___. All rights reserved.
//

#include "QYTSampler.h"

namespace QYT
{
    QYTSampler::QYTSampler(int xstart, int xend, int ystart, int yend,
                           int spp, QYTReal sopen, QYTReal sclose):
                xPixelStart(xstart), xPixelEnd(xend), yPixelStart(ystart),
                yPixelEnd(yend), samplesPerPixel(spp), shutterOpen(sopen),
                shutterClose(sclose)
    { }
    
    QYTSampler::~QYTSampler()
    {}
    
    void QYTSampler::computeSubWindow(int num, int count, int *xstart, int *xend, int *ystart, int *yend) const
    {
        // Determine how many tiles to use in each dimension, _nx_ and _ny_
        int dx = xPixelEnd - xPixelStart, dy = yPixelEnd - yPixelStart;
        int nx = count, ny = 1;
        while ((nx & 0x1) == 0 && 2 * dx * ny < dy * nx) {
            nx >>= 1;
            ny <<= 1;
        }
        Assert(nx * ny == count);
        
        // Compute $x$ and $y$ pixel sample range for sub-window
        int xo = num % nx, yo = num / nx;
        float tx0 = float(xo) / float(nx), tx1 = float(xo+1) / float(nx);
        float ty0 = float(yo) / float(ny), ty1 = float(yo+1) / float(ny);
        *xstart = QYTFloor2Int(QYTLerp(tx0, xPixelStart, xPixelEnd));
        *xend   = QYTFloor2Int(QYTLerp(tx1, xPixelStart, xPixelEnd));
        *ystart = QYTFloor2Int(QYTLerp(ty0, yPixelStart, yPixelEnd));
        *yend   = QYTFloor2Int(QYTLerp(ty1, yPixelStart, yPixelEnd));
    }
    
    void QYTSample::allocateSampleMemory()
    {
        if(oneD != nullptr)
        {
            int nPtrs = (uint32_t)(n1D.size() + n2D.size());
            for (int i = 0; i < nPtrs; ++i)
                delete [] oneD[i];
        }
        
        int nPtrs = (uint32_t)(n1D.size() + n2D.size());
        if (!nPtrs)
        {
            oneD = twoD = nullptr;
            return;
        }
 
        oneD = new QYTReal*[nPtrs];
        twoD = oneD + n1D.size();
        
        //计算总共需要的采样数
        size_t totSamples = 0;
        for (uint32_t i = 0; i < n1D.size(); ++i)
            totSamples += n1D[i];
        for (uint32_t i = 0; i < n2D.size(); ++i)
            totSamples += 2 * n2D[i];
        
        QYTReal* mem = new QYTReal[totSamples];
        for (uint32_t i = 0; i < n1D.size(); ++i) {
            oneD[i] = mem;
            mem += n1D[i];
        }
        for (uint32_t i = 0; i < n2D.size(); ++i) {
            twoD[i] = mem;
            mem += 2 * n2D[i];
        }

    }
    
    QYTSample* QYTSample::copy(int count) const
    {
        QYTSample *ret = new QYTSample[count];
        for (int i = 0; i < count; ++i) {
            ret[i].n1D = n1D;
            ret[i].n2D = n2D;
            ret[i].allocateSampleMemory();
        }
        return ret;
    }
    
    void QYTStratifiedSample2D(float *samp, int nx, int ny,
                               const QYTRNG& rng,
                               bool jitter)
    {
        float dx = 1.f / nx, dy = 1.f / ny;
        
        for (int y = 0; y < ny; ++y)
        {
            for (int x = 0; x < nx; ++x)
            {
                float jx = jitter ? rng.randomFloat(0.f, 1.f) : 0.5f;
                float jy = jitter ? rng.randomFloat(0.f, 1.f) : 0.5f;
                *samp++ = std::min((x + jx) * dx, OneMinusEpsilon);
                *samp++ = std::min((y + jy) * dy, OneMinusEpsilon);
            }
        }
    }
    
    void QYTStratifiedSample1D(float *samp, int nSamples,
                               const QYTRNG& rng,
                               bool jitter)
    {
        float invTot = 1.f / nSamples;
        for (int i = 0;  i < nSamples; ++i) {
            float delta = jitter ? rng.randomFloat(0.f, 1.f) : 0.5f;
            *samp++ = std::min((i + delta) * invTot, OneMinusEpsilon);
        }
    }
    
    void QYTLatinHypercube(float *samples, uint32_t nSamples, uint32_t nDim,
                           const QYTRNG &rng)
    {
        // Generate LHS samples along diagonal
        float delta = 1.f / nSamples;
        for (uint32_t i = 0; i < nSamples; ++i)
            for (uint32_t j = 0; j < nDim; ++j)
                samples[nDim * i + j] = std::min((i + (rng.randomFloat(0.f, 1.f))) * delta,
                                            OneMinusEpsilon);
        
        // Permute LHS samples in each dimension
        for (uint32_t i = 0; i < nDim; ++i) {
            for (uint32_t j = 0; j < nSamples; ++j) {
                uint32_t other = j + (rng.randomUInt(0, RAND_MAX) % (nSamples - j));
                std::swap(samples[nDim * j + i], samples[nDim * other + i]);
            }
        }
    }
    
}