#ifndef CONST_BUFFER_H
#define CONST_BUFFER_H

#include <string>
#include <unordered_map>
#include <cassert>

struct Variable
{
	size_t Offset;
	size_t Size;

	bool operator==(const Variable& rhs) const;
	bool operator!=(const Variable& rhs) const;
};

enum ConstBufferBindStage
{
	CB_UNDEFINE_BIND_STAGE = 0,
	CB_VERTEX_BIND_STAGE   = 1 << 0,
	CB_PIXEL_BIND_STAGE    = 1 << 1
};

class ConstBuffer
{
public:
	ConstBuffer(const std::string& name, unsigned int slot, unsigned int binding, size_t size);
	ConstBuffer(const ConstBuffer& constBuffer);
	~ConstBuffer();

	bool operator==(const ConstBuffer& rhs) const;
	bool operator!=(const ConstBuffer& rhs) const;

	unsigned int GetSlot() const;
	unsigned int GetBinding() const;
	const unsigned char* GetData() const;
	int GetSize() const;
	int GetUsedSize() const;
	ConstBufferBindStage GetBindStage() const;

	void AddBindStage(ConstBufferBindStage bindStage);
	void AddVariable(const std::string& name, const Variable& variable);

	template<typename T>
	void SetVariable(const std::string& name, const T& src)
	{
		if(mVariables.find(name) != mVariables.end())
		{
			const Variable& variable = mVariables[name];
			assert(variable.Size == sizeof(T));
			T *dst = reinterpret_cast<T *>(mBuffer + variable.Offset);
			memcpy(dst, &src, variable.Size);
		}
	}
private:
	std::string mName;
	unsigned int mSlot;
	unsigned int mBinding;
	unsigned char *mBuffer;
	size_t mSize;
	size_t mUsedSize;
	ConstBufferBindStage mBindStage;
	std::unordered_map<std::string, Variable> mVariables;
};

#endif