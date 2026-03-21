#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"

#include "Bindable.h"

class VertexBuffer : public Bindable
{
public:
	VertexBuffer(void *data, size_t count, size_t stride);
	VertexBuffer(const VertexBuffer* vertexBuffer);
	virtual ~VertexBuffer();

    int GetVertexSize() const;
    int GetVertexCount() const;
    unsigned char* GetData() const;
private:
    VertexBuffer();
    int mVertexSize;
    int mVertexCount;
    unsigned char* mData;
};

#endif