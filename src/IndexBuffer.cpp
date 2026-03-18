#include "IndexBuffer.h"
#include <cassert>

IndexBuffer::IndexBuffer()
{
	mIndexQuantity = 0;
	mIndices = nullptr;
}

IndexBuffer::IndexBuffer(void *data, size_t count)
{
	mIndexQuantity = count;
	mIndices = new int[mIndexQuantity];
	memcpy(mIndices, data, mIndexQuantity * sizeof(int));
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