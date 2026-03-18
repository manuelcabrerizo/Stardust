#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#include "Bindable.h"

class IndexBuffer : public Bindable
{
public:

	IndexBuffer(void *data, size_t count);
	IndexBuffer(const IndexBuffer* indexBuffer);
	virtual ~IndexBuffer();

	int GetIndexQuantity() const;
	int* GetData();
	const int* GetData() const;

	void SetIndexQuantity(int indexQuantity);
protected:
	IndexBuffer();

	int mIndexQuantity;
	int* mIndices;
};

#endif