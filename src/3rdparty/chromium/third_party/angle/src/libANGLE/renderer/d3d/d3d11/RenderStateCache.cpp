//
// Copyright 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderStateCache.cpp: Defines rx::RenderStateCache, a cache of Direct3D render
// state objects.

#include "libANGLE/renderer/d3d/d3d11/RenderStateCache.h"

#include <float.h>

#include "common/Color.h"
#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/renderer/d3d/d3d11/Context11.h"
#include "libANGLE/renderer/d3d/d3d11/Framebuffer11.h"
#include "libANGLE/renderer/d3d/d3d11/Renderer11.h"
#include "libANGLE/renderer/d3d/d3d11/renderer11_utils.h"

namespace rx
{
using namespace gl_d3d11;

RenderStateCache::RenderStateCache()
    : mBlendStateCache(kMaxStates),
      mRasterizerStateCache(kMaxStates),
      mDepthStencilStateCache(kMaxStates),
      mSamplerStateCache(kMaxStates)
{}

RenderStateCache::~RenderStateCache() {}

void RenderStateCache::clear()
{
    mBlendStateCache.Clear();
    mRasterizerStateCache.Clear();
    mDepthStencilStateCache.Clear();
    mSamplerStateCache.Clear();
}

// static
d3d11::BlendStateKey RenderStateCache::GetBlendStateKey(const gl::Context *context,
                                                        Framebuffer11 *framebuffer11,
                                                        const gl::BlendStateArray &blendStateArray,
                                                        bool sampleAlphaToCoverage)
{
    d3d11::BlendStateKey key;
    // All elements of the BlendStateArray inside the key should be initialized for the caching to
    // work correctly. Due to mrt_perf_workaround, the actual indices of active draw buffers may be
    // different, so both arrays should be tracked.
    const gl::AttachmentList &colorbuffers = framebuffer11->getColorAttachmentsForRender(context);
    const gl::DrawBufferMask colorAttachmentsForRenderMask =
        framebuffer11->getLastColorAttachmentsForRenderMask();

    ASSERT(blendStateArray.size() == colorAttachmentsForRenderMask.size());
    ASSERT(colorbuffers.size() == colorAttachmentsForRenderMask.count());

    size_t keyBlendIndex = 0;
    for (size_t sourceIndex : colorAttachmentsForRenderMask)
    {
        const gl::BlendState &blendState = blendStateArray[sourceIndex];
        gl::BlendState &keyBlendState    = key.blendStateArray[keyBlendIndex];

        // With blending disabled, factors and equations are ignored when building
        // D3D11_RENDER_TARGET_BLEND_DESC, so we can reduce the amount of unique keys by
        // enforcing default values.
        keyBlendState = gl::BlendState();

        if (blendState.blend)
        {
            keyBlendState.blend              = true;
            keyBlendState.sourceBlendRGB     = blendState.sourceBlendRGB;
            keyBlendState.sourceBlendAlpha   = blendState.sourceBlendAlpha;
            keyBlendState.destBlendRGB       = blendState.destBlendRGB;
            keyBlendState.destBlendAlpha     = blendState.destBlendAlpha;
            keyBlendState.blendEquationRGB   = blendState.blendEquationRGB;
            keyBlendState.blendEquationAlpha = blendState.blendEquationAlpha;
        }

        // These values are used only for caching (hash calculation) purposes.
        // Actual write color mask value is derived below.
        keyBlendState.colorMaskRed   = blendState.colorMaskRed;
        keyBlendState.colorMaskGreen = blendState.colorMaskGreen;
        keyBlendState.colorMaskBlue  = blendState.colorMaskBlue;
        keyBlendState.colorMaskAlpha = blendState.colorMaskAlpha;

        ASSERT(keyBlendIndex < colorbuffers.size());
        const gl::FramebufferAttachment *attachment = colorbuffers[keyBlendIndex];

        if (attachment)  // when mrt_perf_workaround is disabled, nullptr attachments may exist
        {
            key.rtvMax = static_cast<uint16_t>(keyBlendIndex) + 1;
            key.rtvMasks[keyBlendIndex] =
                (gl_d3d11::GetColorMask(*attachment->getFormat().info)) &
                gl_d3d11::ConvertColorMask(blendState.colorMaskRed, blendState.colorMaskGreen,
                                           blendState.colorMaskBlue, blendState.colorMaskAlpha);
        }
        keyBlendIndex++;
    }

    // Initialize remaining slots in key.blendStateArray
    if (keyBlendIndex < key.blendStateArray.size())
    {
        std::fill(key.blendStateArray.begin() + keyBlendIndex, key.blendStateArray.end(),
                  gl::BlendState());
    }

    key.sampleAlphaToCoverage = sampleAlphaToCoverage ? 1 : 0;
    return key;
}

angle::Result RenderStateCache::getBlendState(const gl::Context *context,
                                              Renderer11 *renderer,
                                              const d3d11::BlendStateKey &key,
                                              const d3d11::BlendState **outBlendState)
{
    auto keyIter = mBlendStateCache.Get(key);
    if (keyIter != mBlendStateCache.end())
    {
        *outBlendState = &keyIter->second;
        return angle::Result::Continue;
    }

    TrimCache(kMaxStates, kGCLimit, "blend state", &mBlendStateCache);

    // Create a new blend state and insert it into the cache
    D3D11_BLEND_DESC blendDesc;
    const gl::BlendStateArray &blendStateArray = key.blendStateArray;

    blendDesc.AlphaToCoverageEnable  = key.sampleAlphaToCoverage != 0 ? TRUE : FALSE;
    blendDesc.IndependentBlendEnable = key.rtvMax > 1 ? TRUE : FALSE;

    // D3D11 API always accepts an array of blend states. Its validity depends on the hardware
    // feature level. Given that we do not expose GL entrypoints that set per-buffer blend states on
    // systems lower than FL10_1, this array will be always valid.

    ASSERT(blendStateArray.size() >= D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
    for (size_t i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        D3D11_RENDER_TARGET_BLEND_DESC &rtDesc = blendDesc.RenderTarget[i];

        rtDesc = {};  // otherwise its content may be undefined

        const gl::BlendState &blendState = blendStateArray[i];
        if (blendState.blend)
        {
            rtDesc.BlendEnable    = true;
            rtDesc.SrcBlend       = gl_d3d11::ConvertBlendFunc(blendState.sourceBlendRGB, false);
            rtDesc.DestBlend      = gl_d3d11::ConvertBlendFunc(blendState.destBlendRGB, false);
            rtDesc.BlendOp        = gl_d3d11::ConvertBlendOp(blendState.blendEquationRGB);
            rtDesc.SrcBlendAlpha  = gl_d3d11::ConvertBlendFunc(blendState.sourceBlendAlpha, true);
            rtDesc.DestBlendAlpha = gl_d3d11::ConvertBlendFunc(blendState.destBlendAlpha, true);
            rtDesc.BlendOpAlpha   = gl_d3d11::ConvertBlendOp(blendState.blendEquationAlpha);
        }

        rtDesc.RenderTargetWriteMask = key.rtvMasks[i];
    }

    d3d11::BlendState d3dBlendState;
    ANGLE_TRY(renderer->allocateResource(GetImplAs<Context11>(context), blendDesc, &d3dBlendState));
    const auto &iter = mBlendStateCache.Put(key, std::move(d3dBlendState));

    *outBlendState = &iter->second;

    return angle::Result::Continue;
}

angle::Result RenderStateCache::getRasterizerState(const gl::Context *context,
                                                   Renderer11 *renderer,
                                                   const gl::RasterizerState &rasterState,
                                                   bool scissorEnabled,
                                                   ID3D11RasterizerState **outRasterizerState)
{
    d3d11::RasterizerStateKey key;
    key.rasterizerState = rasterState;
    key.scissorEnabled  = scissorEnabled ? 1 : 0;

    auto keyIter = mRasterizerStateCache.Get(key);
    if (keyIter != mRasterizerStateCache.end())
    {
        *outRasterizerState = keyIter->second.get();
        return angle::Result::Continue;
    }

    TrimCache(kMaxStates, kGCLimit, "rasterizer state", &mRasterizerStateCache);

    D3D11_CULL_MODE cullMode =
        gl_d3d11::ConvertCullMode(rasterState.cullFace, rasterState.cullMode);

    // Disable culling if drawing points
    if (rasterState.pointDrawMode)
    {
        cullMode = D3D11_CULL_NONE;
    }

    D3D11_RASTERIZER_DESC rasterDesc;
    rasterDesc.FillMode              = D3D11_FILL_SOLID;
    rasterDesc.CullMode              = cullMode;
    rasterDesc.FrontCounterClockwise = (rasterState.frontFace == GL_CCW) ? FALSE : TRUE;
    rasterDesc.DepthBiasClamp = 0.0f;  // MSDN documentation of DepthBiasClamp implies a value of
                                       // zero will preform no clamping, must be tested though.
    rasterDesc.DepthClipEnable       = TRUE;
    rasterDesc.ScissorEnable         = scissorEnabled ? TRUE : FALSE;
    rasterDesc.MultisampleEnable     = rasterState.multiSample;
    rasterDesc.AntialiasedLineEnable = FALSE;

    if (rasterState.polygonOffsetFill)
    {
        rasterDesc.SlopeScaledDepthBias = rasterState.polygonOffsetFactor;
        rasterDesc.DepthBias            = (INT)rasterState.polygonOffsetUnits;
    }
    else
    {
        rasterDesc.SlopeScaledDepthBias = 0.0f;
        rasterDesc.DepthBias            = 0;
    }

    d3d11::RasterizerState dx11RasterizerState;
    ANGLE_TRY(renderer->allocateResource(GetImplAs<Context11>(context), rasterDesc,
                                         &dx11RasterizerState));
    *outRasterizerState = dx11RasterizerState.get();
    mRasterizerStateCache.Put(key, std::move(dx11RasterizerState));

    return angle::Result::Continue;
}

angle::Result RenderStateCache::getDepthStencilState(const gl::Context *context,
                                                     Renderer11 *renderer,
                                                     const gl::DepthStencilState &glState,
                                                     const d3d11::DepthStencilState **outDSState)
{
    auto keyIter = mDepthStencilStateCache.Get(glState);
    if (keyIter != mDepthStencilStateCache.end())
    {
        *outDSState = &keyIter->second;
        return angle::Result::Continue;
    }

    TrimCache(kMaxStates, kGCLimit, "depth stencil state", &mDepthStencilStateCache);

    D3D11_DEPTH_STENCIL_DESC dsDesc     = {};
    dsDesc.DepthEnable                  = glState.depthTest ? TRUE : FALSE;
    dsDesc.DepthWriteMask               = ConvertDepthMask(glState.depthMask);
    dsDesc.DepthFunc                    = ConvertComparison(glState.depthFunc);
    dsDesc.StencilEnable                = glState.stencilTest ? TRUE : FALSE;
    dsDesc.StencilReadMask              = ConvertStencilMask(glState.stencilMask);
    dsDesc.StencilWriteMask             = ConvertStencilMask(glState.stencilWritemask);
    dsDesc.FrontFace.StencilFailOp      = ConvertStencilOp(glState.stencilFail);
    dsDesc.FrontFace.StencilDepthFailOp = ConvertStencilOp(glState.stencilPassDepthFail);
    dsDesc.FrontFace.StencilPassOp      = ConvertStencilOp(glState.stencilPassDepthPass);
    dsDesc.FrontFace.StencilFunc        = ConvertComparison(glState.stencilFunc);
    dsDesc.BackFace.StencilFailOp       = ConvertStencilOp(glState.stencilBackFail);
    dsDesc.BackFace.StencilDepthFailOp  = ConvertStencilOp(glState.stencilBackPassDepthFail);
    dsDesc.BackFace.StencilPassOp       = ConvertStencilOp(glState.stencilBackPassDepthPass);
    dsDesc.BackFace.StencilFunc         = ConvertComparison(glState.stencilBackFunc);

    d3d11::DepthStencilState dx11DepthStencilState;
    ANGLE_TRY(
        renderer->allocateResource(GetImplAs<Context11>(context), dsDesc, &dx11DepthStencilState));
    const auto &iter = mDepthStencilStateCache.Put(glState, std::move(dx11DepthStencilState));

    *outDSState = &iter->second;

    return angle::Result::Continue;
}

angle::Result RenderStateCache::getSamplerState(const gl::Context *context,
                                                Renderer11 *renderer,
                                                const gl::SamplerState &samplerState,
                                                ID3D11SamplerState **outSamplerState)
{
    auto keyIter = mSamplerStateCache.Get(samplerState);
    if (keyIter != mSamplerStateCache.end())
    {
        *outSamplerState = keyIter->second.get();
        return angle::Result::Continue;
    }

    TrimCache(kMaxStates, kGCLimit, "sampler state", &mSamplerStateCache);

    const auto &featureLevel = renderer->getRenderer11DeviceCaps().featureLevel;

    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter =
        gl_d3d11::ConvertFilter(samplerState.getMinFilter(), samplerState.getMagFilter(),
                                samplerState.getMaxAnisotropy(), samplerState.getCompareMode());
    samplerDesc.AddressU   = gl_d3d11::ConvertTextureWrap(samplerState.getWrapS());
    samplerDesc.AddressV   = gl_d3d11::ConvertTextureWrap(samplerState.getWrapT());
    samplerDesc.AddressW   = gl_d3d11::ConvertTextureWrap(samplerState.getWrapR());
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy =
        gl_d3d11::ConvertMaxAnisotropy(samplerState.getMaxAnisotropy(), featureLevel);
    samplerDesc.ComparisonFunc = gl_d3d11::ConvertComparison(samplerState.getCompareFunc());
    angle::ColorF borderColor;
    if (samplerState.getBorderColor().type == angle::ColorGeneric::Type::Float)
    {
        borderColor = samplerState.getBorderColor().colorF;
    }
    samplerDesc.BorderColor[0] = borderColor.red;
    samplerDesc.BorderColor[1] = borderColor.green;
    samplerDesc.BorderColor[2] = borderColor.blue;
    samplerDesc.BorderColor[3] = borderColor.alpha;
    samplerDesc.MinLOD         = samplerState.getMinLod();
    samplerDesc.MaxLOD         = samplerState.getMaxLod();

    if (featureLevel <= D3D_FEATURE_LEVEL_9_3)
    {
        // Check that maxLOD is nearly FLT_MAX (1000.0f is the default), since 9_3 doesn't support
        // anything other than FLT_MAX. Note that Feature Level 9_* only supports GL ES 2.0, so the
        // consumer of ANGLE can't modify the Max LOD themselves.
        ASSERT(samplerState.getMaxLod() >= 999.9f);

        // Now just set MaxLOD to FLT_MAX. Other parts of the renderer (e.g. the non-zero max LOD
        // workaround) should take account of this.
        samplerDesc.MaxLOD = FLT_MAX;
    }

    d3d11::SamplerState dx11SamplerState;
    ANGLE_TRY(
        renderer->allocateResource(GetImplAs<Context11>(context), samplerDesc, &dx11SamplerState));
    *outSamplerState = dx11SamplerState.get();
    mSamplerStateCache.Put(samplerState, std::move(dx11SamplerState));

    return angle::Result::Continue;
}

}  // namespace rx