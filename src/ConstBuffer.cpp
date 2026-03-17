#include "ConstBuffer.h"
#include <cstdlib>

bool Variable::operator==(const Variable& rhs) const
{
	return Offset == rhs.Offset && Size == rhs.Size;
}

bool Variable::operator!=(const Variable& rhs) const
{
	return !(*this == rhs);
}

ConstBuffer::ConstBuffer(const std::string& name, unsigned int slot, unsigned int binding, size_t size)
{
	mBindStage = CB_UNDEFINE_BIND_STAGE;
	mName = name;
	mSlot = slot;
	mBinding = binding;
	mUsedSize = 0;
	mSize = size;
	mBuffer = static_cast<unsigned char*>(_aligned_malloc(mSize, 16));
	if(mBuffer)
	{
		memset(mBuffer, 0, mSize);
	}
}

ConstBuffer::ConstBuffer(const ConstBuffer& constBuffer)
{
	mName = constBuffer.mName;
	mSlot = constBuffer.mSlot;
	mBinding = constBuffer.mBinding;
	mUsedSize = constBuffer.mUsedSize;
	mSize = constBuffer.mSize;
	mBindStage = constBuffer.mBindStage;
	mBuffer = static_cast<unsigned char*>(_aligned_malloc(mSize, 16));
	if(mBuffer)
	{
		memcpy(mBuffer, constBuffer.mBuffer, mSize);
	}
	mVariables = constBuffer.mVariables;
}

ConstBuffer::~ConstBuffer()
{
	mSize = 0;
	if(mBuffer)
	{
		_aligned_free(mBuffer);
	}
}

bool ConstBuffer::operator==(const ConstBuffer& rhs) const
{
	if(mSlot != rhs.mSlot)
	{
		return false;
	}
	if(mBinding != rhs.mBinding)
	{
		return false;
	}
	if(mSize != rhs.mSize)
	{
		return false;
	}
	if(mName.compare(rhs.mName) != 0)
	{
		return false;
	}
	if(mVariables != rhs.mVariables)
	{
		return false;
	}
	return true;
}

bool ConstBuffer::operator!=(const ConstBuffer& rhs) const
{
	return !(*this == rhs);
}

unsigned int ConstBuffer::GetSlot() const
{
	return mSlot;
}

unsigned int ConstBuffer::GetBinding() const
{
	return mBinding;
}

const unsigned char* ConstBuffer::GetData() const
{
	return mBuffer;
}

int ConstBuffer::GetSize() const
{
	return mSize;
}

int ConstBuffer::GetUsedSize() const
{
	return mUsedSize;
}

ConstBufferBindStage ConstBuffer::GetBindStage() const
{
	return mBindStage;	
}

void ConstBuffer::AddBindStage(ConstBufferBindStage bindStage)
{
	mBindStage = static_cast<ConstBufferBindStage>(mBindStage | bindStage);
}

void ConstBuffer::AddVariable(const std::string& name, const Variable& variable)
{
	assert(mVariables.find(name) == mVariables.end());
	assert(mUsedSize + variable.Size <= mSize);
	mVariables.emplace(name, variable);
	mUsedSize += variable.Size;
}