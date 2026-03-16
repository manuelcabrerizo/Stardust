#include "VertexBuffer.h"
#include <cassert>

VertexBuffer::VertexBuffer(void *data, size_t count, size_t stride)
    : mVertexSize(stride), mVertexCount(count)
{
    assert(mVertexCount > 0);
    mData = new unsigned char[mVertexSize * mVertexCount];
    memcpy(mData, data, mVertexSize * mVertexCount);
}

VertexBuffer::VertexBuffer(const VertexBuffer* vBuffer)
{
    assert(vBuffer);
    mVertexSize = vBuffer->mVertexSize;
    mVertexCount = vBuffer->mVertexCount;
    mData = new unsigned char[mVertexSize * mVertexCount];
    memcpy(mData, vBuffer->mData, mVertexSize * mVertexCount);
}

VertexBuffer::VertexBuffer()
{
    mVertexCount = 0;
    mVertexSize = 0;
    mData = nullptr;
}

VertexBuffer::~VertexBuffer()
{
    Release();
    delete[] mData;
}

int VertexBuffer::GetVertexSize() const
{
    return mVertexSize;
}

int VertexBuffer::GetVertexCount() const
{
    return mVertexCount;
}

unsigned char* VertexBuffer::GetData() const
{
    return mData;
}