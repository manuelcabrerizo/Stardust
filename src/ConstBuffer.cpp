#include "ConstBuffer.h"
#include <cstdlib>

// Source: https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0#
//[Do's and Don'ts for updating constant buffers useing DX10/DX11.0]
// 1: Try to update only one constant buffer in between draw class. This constant buffer should only
// be bound to a single  shader stage, if possible. Where this is not possible, only set and update
// the absolute minimum number of constant buffer necessary to do the job.
// 2: Update constant buffers using one of this two methods:
// 		a: Map(MAP_WRITE_DIRCARD) -> memcopy() the system memory data -> Unmap()
//		b: UpdateSubresource()
//		   NVidia drivers map UpdateSubresource to Map(MAP_WRITE_DIRCARD) -> memcopy() -> Unmap(). this means that you will see the cost
//		   of the memcpy if you profile the CPU cost of UpdateSubresource
// 3: Don't update a subset of a large constant buffer, as this increase the accumulated memory size more than necessary, and will cause
// the driver to reach the renaming limit more quickli. This piece of advice doesn't apply when using the DX11.1 features that allow for
// partial constant buffer updates.
// 4: Avoid switching between differents constant buffers, as often as possible. Be aware of the cost for a cache miss of a fresh constatn buffer

//[Updating constant buffers using DX11.1 and above]
// DX11.1 adds API features that allow an application to manage constant buffer memory directly. It allows the creation of big constant buffers
// that can be filled with data for multiple rendering operation in one go.
// This is possible in DX11.1, as the constant buffers for shader stages can be set in a way that only a subsection of a large constant buffer ends,
// up being utilized by a shader - e.g. by using PSSetConstantConstantBuffers1() for pixel stage
// This basically allows the programmer to work around the renaming limit inside the driver. In an extreme case, one can prepare a big constant buffer
// that contains all the constants for one frame's worth of rendering
// This would lead to the following optimal usage pattern:
// 		1. Map(MAP_WRITE_DIRCARD) a big constant buffer
// 		2. For all remaining draw calls - initially this is all of the draw calls
//			a. Append per draw call constant to the mapped memory
// 			b. If constant buffer is full exit loop
//		3. Unmap() the big constant buffer
//		4. For all draw calls that step 2 has generated constant data for
//			a. Set the right part of the big buffer as a constant buffer XXSetConstantBuffers1()
//			b. Perform draw call
//		5. If all draw calls have not been processed restart at 1.
// Alternatively one can use the following pattern
//		1. Use UpdateSubresource1(WRITE_DISCARD) to copy the constant data for the first of the remaining draw calls - initially this is all of them
//		2. For all remaining draw calls – initially this is all of them minus the first
//			a. Use UpdateSubresource1(NOOVERWRITE) to copy constants for the current draw call
//			b. If constant buffer is full exit loop.
//		3. For all draw calls that step 2 has generated constant data for
//			a. set the right part of the big buffer as a constant buffer XXSetConstantBuffers1()
//			b. Perform draw call
//		4. If not all draw calls have been processed restart at 1.
// All constant buffer data that is passed to XXSetConstantBuffers1() needs to be aligned to 256 byte boundaries. You need to make sure that you update
// the memory inside the big constant buffers accordingly.


// For D3D11 use _aligned_malloc() to allocate merory that is aligned to 16 byte boundaries.
// this speeds up the necessary memcpy() operation from application memory to the memory
// returned by Map(). In a similar throw UpdateSubresource() will be able to perfom the
// copy operation faster too. https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0#
//mBuffer = new unsigned char[mSize];

bool Variable::operator==(const Variable& rhs) const
{
	return Offset == rhs.Offset && Size == rhs.Size;
}

bool Variable::operator!=(const Variable& rhs) const
{
	return !(*this == rhs);
}

ConstBuffer::ConstBuffer(const std::string& name, unsigned int slot, size_t size)
{
	mBindStage = CB_UNDEFINE_BIND_STAGE;
	mName = name;
	mSlot = slot;
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