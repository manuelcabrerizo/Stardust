#include "IndexBuffer.h"
#include <cassert>

IndexBuffer::IndexBuffer()
{
	mIndexQuantity = 0;
	mIndices = nullptr;
}

IndexBuffer::IndexBuffer(int indexQuantity)
{
	mIndexQuantity = indexQuantity;
	mIndices = new int[mIndexQuantity];
	memset(mIndices, 0, mIndexQuantity * sizeof(int));
}

IndexBuffer::IndexBuffer(const IndexBuffer* indexBuffer)
{
	assert(indexBuffer);
	mIndexQuantity = indexBuffer->mIndexQuantity;
	mIndices = new int[mIndexQuantity];
	memcpy(mIndices, indexBuffer->mIndices, mIndexQuantity * sizeof(int));
}

IndexBuffer::~IndexBuffer()
{
	Release();
	delete[] mIndices;
}

int IndexBuffer::operator[](int i) const
{
	assert(i >= 0 && i < mIndexQuantity);
	return mIndices[i];
}

int& IndexBuffer::operator[](int i)
{
	assert(i >= 0 && i < mIndexQuantity);
	return mIndices[i];
}

int IndexBuffer::GetIndexQuantity() const
{
	return mIndexQuantity;
}

int* IndexBuffer::GetData()
{
	return mIndices;
}

const int* IndexBuffer::GetData() const
{
	return mIndices;
}

void IndexBuffer::SetIndexQuantity(int indexQuantity)
{
	mIndexQuantity = indexQuantity;
}