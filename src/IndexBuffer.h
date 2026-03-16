#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#include "Bindable.h"

class IndexBuffer : public Bindable
{
public:

	IndexBuffer(int indexQuantity);
	IndexBuffer(const IndexBuffer* indexBuffer);
	virtual ~IndexBuffer();

	// Access to indices
	int operator[](int i) const;
	int& operator[](int i);

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