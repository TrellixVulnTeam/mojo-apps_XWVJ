/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GrProcessor.h"
#include "GrBackendProcessorFactory.h"
#include "GrContext.h"
#include "GrCoordTransform.h"
#include "GrGeometryData.h"
#include "GrInvariantOutput.h"
#include "GrMemoryPool.h"
#include "SkTLS.h"

#if SK_ALLOW_STATIC_GLOBAL_INITIALIZERS

/*
 * Originally these were both in the processor unit test header, but then it seemed to cause linker
 * problems on android.
 */
template<>
SkTArray<GrProcessorTestFactory<GrFragmentProcessor>*, true>*
GrProcessorTestFactory<GrFragmentProcessor>::GetFactories() {
    static SkTArray<GrProcessorTestFactory<GrFragmentProcessor>*, true> gFactories;
    return &gFactories;
}

template<>
SkTArray<GrProcessorTestFactory<GrXferProcessor>*, true>*
GrProcessorTestFactory<GrXferProcessor>::GetFactories() {
    static SkTArray<GrProcessorTestFactory<GrXferProcessor>*, true> gFactories;
    return &gFactories;
}

template<>
SkTArray<GrProcessorTestFactory<GrGeometryProcessor>*, true>*
GrProcessorTestFactory<GrGeometryProcessor>::GetFactories() {
    static SkTArray<GrProcessorTestFactory<GrGeometryProcessor>*, true> gFactories;
    return &gFactories;
}

/*
 * To ensure we always have successful static initialization, before creating from the factories
 * we verify the count is as expected.  If a new factory is added, then these numbers must be
 * manually adjusted.
 */
static const int kFPFactoryCount = 37;
static const int kGPFactoryCount = 14;
static const int kXPFactoryCount = 0;

template<>
void GrProcessorTestFactory<GrFragmentProcessor>::VerifyFactoryCount() {
    if (kFPFactoryCount != GetFactories()->count()) {
        SkFAIL("Wrong number of fragment processor factories!");
    }
}

template<>
void GrProcessorTestFactory<GrGeometryProcessor>::VerifyFactoryCount() {
    if (kGPFactoryCount != GetFactories()->count()) {
        SkFAIL("Wrong number of geometry processor factories!");
    }
}

template<>
void GrProcessorTestFactory<GrXferProcessor>::VerifyFactoryCount() {
    if (kXPFactoryCount != GetFactories()->count()) {
        SkFAIL("Wrong number of xfer processor factories!");
    }
}

#endif

namespace GrProcessorUnitTest {
const SkMatrix& TestMatrix(SkRandom* random) {
    static SkMatrix gMatrices[5];
    static bool gOnce;
    if (!gOnce) {
        gMatrices[0].reset();
        gMatrices[1].setTranslate(SkIntToScalar(-100), SkIntToScalar(100));
        gMatrices[2].setRotate(SkIntToScalar(17));
        gMatrices[3].setRotate(SkIntToScalar(185));
        gMatrices[3].postTranslate(SkIntToScalar(66), SkIntToScalar(-33));
        gMatrices[3].postScale(SkIntToScalar(2), SK_ScalarHalf);
        gMatrices[4].setRotate(SkIntToScalar(215));
        gMatrices[4].set(SkMatrix::kMPersp0, 0.00013f);
        gMatrices[4].set(SkMatrix::kMPersp1, -0.000039f);
        gOnce = true;
    }
    return gMatrices[random->nextULessThan(static_cast<uint32_t>(SK_ARRAY_COUNT(gMatrices)))];
}
}

class GrProcessor_Globals {
public:
    static GrMemoryPool* GetTLS() {
        return (GrMemoryPool*)SkTLS::Get(CreateTLS, DeleteTLS);
    }

private:
    static void* CreateTLS() {
        return SkNEW_ARGS(GrMemoryPool, (4096, 4096));
    }

    static void DeleteTLS(void* pool) {
        SkDELETE(reinterpret_cast<GrMemoryPool*>(pool));
    }
};

int32_t GrBackendProcessorFactory::fCurrProcessorClassID =
        GrBackendProcessorFactory::kIllegalProcessorClassID;

///////////////////////////////////////////////////////////////////////////////

GrProcessor::~GrProcessor() {}

const char* GrProcessor::name() const {
    return this->getFactory().name();
}

void GrProcessor::addTextureAccess(const GrTextureAccess* access) {
    fTextureAccesses.push_back(access);
    this->addGpuResource(access->getProgramTexture());
}

void* GrProcessor::operator new(size_t size) {
    return GrProcessor_Globals::GetTLS()->allocate(size);
}

void GrProcessor::operator delete(void* target) {
    GrProcessor_Globals::GetTLS()->release(target);
}

bool GrProcessor::hasSameTextureAccesses(const GrProcessor& that) const {
    if (this->numTextures() != that.numTextures()) {
        return false;
    }
    for (int i = 0; i < this->numTextures(); ++i) {
        if (this->textureAccess(i) != that.textureAccess(i)) {
            return false;
        }
    }
    return true;
}

void GrProcessor::computeInvariantOutput(GrInvariantOutput* inout) const {
    this->onComputeInvariantOutput(inout);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void GrFragmentProcessor::addCoordTransform(const GrCoordTransform* transform) {
    fCoordTransforms.push_back(transform);
    SkDEBUGCODE(transform->setInProcessor();)
}

bool GrFragmentProcessor::hasSameTransforms(const GrFragmentProcessor& that) const {
    if (fCoordTransforms.count() != that.fCoordTransforms.count()) {
        return false;
    }
    int count = fCoordTransforms.count();
    for (int i = 0; i < count; ++i) {
        if (*fCoordTransforms[i] != *that.fCoordTransforms[i]) {
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * GrGeometryData shares the same pool so it lives in this file too
 */
void* GrGeometryData::operator new(size_t size) {
    return GrProcessor_Globals::GetTLS()->allocate(size);
}

void GrGeometryData::operator delete(void* target) {
    GrProcessor_Globals::GetTLS()->release(target);
}

