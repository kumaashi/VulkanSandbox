#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iterator>

enum {
	BUFFER_TYPE_NONE = 0,
	BUFFER_TYPE_SHADER,
	BUFFER_TYPE_VERTEX,
	BUFFER_TYPE_TEXTURE,
	BUFFER_TYPE_RENDERTARGET,
	BUFFER_TYPE_VIEW,
};

struct VertexData {
	float pos[3];
	float nor[3];
	float uv[2];
	float color[4];
	VertexData() {
		memset(this, 0, sizeof(VertexData));
	}
};


struct VectorData {
	float data[4] = {};
};

struct MatrixData {
	float data[16] = {};
	MatrixData() {
		//0 1 2 3
		//4 5 6 7
		//8 9 A B
		//C D E F
		data[0] = data[5] = data[10] = data[15] = 1.0f;
	}
	void Print() {
		for (int i = 0; i < 16; i++) {
			if ( (i % 4) == 0) printf("\n");
			printf("%f, ", data[i]);
		}
	}
};

class VertexBuffer;

class BaseBuffer {
	std::string name;
	int type;
	bool is_update;
	BaseBuffer *child;
public:
	BaseBuffer() {
		name = "";
		is_update = false;
		child = nullptr;
	}

	virtual ~BaseBuffer() {
	}

	BaseBuffer *GetChild() {
		return child;
	}

	void SetChild(BaseBuffer *a) {
		child = a;
	}

	void SetType(int t) {
		type = t;
	}

	int GetType() {
		return type;
	}

	std::string GetName() {
		return name;
	}

	void SetName(const std::string &s) {
		name = s;
	}

	void MarkUpdate() {
		is_update = true;
	}

	void UnmarkUpdate() {
		is_update = false;
	}

	bool IsUpdate() {
		return is_update;
	}
};



class TextureBuffer : public BaseBuffer {
	int width;
	int height;
	int format;
	std::vector<char> buffer;
public:
	enum {
		FORMAT_RGBA8,   //GL_RGBA8
		FORMAT_RGBA32I, //GL_RGBA32I
		FORMAT_MAX,
	};
	TextureBuffer() {
		SetType(BUFFER_TYPE_TEXTURE);
		width = 0;
		height = 0;
		format = FORMAT_RGBA8;
	}

	virtual ~TextureBuffer() {
	}

	void Create(int w, int h, int fmt = FORMAT_RGBA8) {
		width = w;
		height = h;
		format = fmt;
	}

	void SetData(void *data, size_t size) {
		buffer.resize(size);
		memcpy(&buffer[0], data, size);
		MarkUpdate();
	}

	bool GetData(std::vector<char> &dest) {
		if (!buffer.empty()) {
			std::copy(buffer.begin(), buffer.end(), std::back_inserter(dest) );
			return true;
		}
		return false;
	}

	void *GetData() {
		if (!buffer.empty()) {
			return &buffer[0];
		}
		return 0;
	}

	int GetWidth() {
		return width;
	}
	int GetHeight() {
		return height;
	}
	int GetFormat() {
		return format;
	}
};

class RenderTargetBuffer : public BaseBuffer {
	enum {
		MAX = 4,
	};
	TextureBuffer color_texture[MAX] = {};
	TextureBuffer depth_texture = {};
	int width;
	int height;
	int format;
	int count;
	float clear_color[4];
public:
	enum {
		TypeBackBuffer = 0,
		TypeTargetBuffer,
		TypeMax,
	};
	RenderTargetBuffer() {
		SetType(BUFFER_TYPE_RENDERTARGET);
		width = 0;
		height = 0;
		format = TextureBuffer::FORMAT_RGBA8;
		clear_color[0] = 1.0f;
		clear_color[1] = 0.0f;
		clear_color[2] = 1.0f;
		clear_color[3] = 1.0f;
	}

	virtual ~RenderTargetBuffer() {
	}

	void Create(int w, int h, int fmt, int t, int cnt) {
		width = w;
		height = h;
		format = TextureBuffer::FORMAT_RGBA8;
		count = cnt;
		for (int i = 0 ; i < count; i++) {
			char name[0x120];
			sprintf(name, "%s_%d", GetName().c_str(), i);
			color_texture[i].SetName(GetName() + std::string(name));
			color_texture[i].Create(w, h, fmt);
		}
		{
			depth_texture.SetName(GetName() + std::string("_depth"));
			depth_texture.Create(w, h, fmt);
		}
		MarkUpdate();
	}
	int GetWidth() {
		return width;
	}
	int GetHeight() {
		return height;
	}
	int GetFormat() {
		return format;
	}

	TextureBuffer *GetTexture(int i, bool is_depth = false) {
		if (is_depth) {
			return &depth_texture;
		}
		if(i < count) {
			return &color_texture[i];
		}
		return nullptr;
	}

	const int GetCount() {
		return count;
	}

	void SetClearColor(float r, float g, float b, float a) {
		clear_color[0] = r;
		clear_color[1] = g;
		clear_color[2] = b;
		clear_color[3] = a;
	}

	void GetClearColor(float *a) {
		for (int i = 0; i < 4; i++) {
			a[i] = clear_color[i];
		}
	}
};


class ViewBuffer : public BaseBuffer {
public:
	//with scissors
	struct ViewportData {
		float x;
		float y;
		float w;
		float h;
		MatrixData proj;
		MatrixData view;
		MatrixData world;
	};

	ViewBuffer() {
		SetType(BUFFER_TYPE_VIEW);
		rendertarget = 0;
		viewports.clear();
		order = 0;
	}
	
	void Clear() {
		vimap.clear();
		uniform_mat4.clear();
		uniform_vec4.clear();
		viewports.clear();
	}

	virtual ~ViewBuffer() {
		rendertarget = 0;
	}

	void SetVertexBuffer(std::string &name, VertexBuffer *a) {
		vimap[name] = a;
	}

	void DelVertexBuffer(std::string &a) {
		vimap.erase(a);
	}

	void GetVertexBuffer(std::vector<VertexBuffer *> &dest) {
		for (auto & it : vimap) {
			dest.push_back(it.second);
		}
	}

	void GetClearColor(float a[4]) {
		for (int i = 0 ; i < 4; i++) {
			a[i] = clearcolor[i];
		}
	}

	void SetClearColor(float r, float g, float b, float a) {
		clearcolor[0] = r;
		clearcolor[1] = g;
		clearcolor[2] = b;
		clearcolor[3] = a;
	}
	
	void SetWidth(float a) {
		width = a;
	}
	
	void SetHeight(float a) {
		height = a;
	}

	float GetWidth() {
		return width;
	}
	
	float GetHeight() {
		return height;
	}

	void SetOrder(int n) {
		order = n;
	}

	int GetOrder() {
		return order;
	}

	void SetMatrix(const std::string &name, const MatrixData &e) {
		uniform_mat4[name] = e;
	}
	
	MatrixData GetMatrix(const std::string &name) {
		MatrixData ret;
		auto it = uniform_mat4.find(name);
		if(it != uniform_mat4.end()) {
			return it->second;
		}
		return ret;
	}

	void GetMatrix(std::map<std::string, MatrixData> &m) {
		auto it = uniform_mat4.begin();
		auto ite = uniform_mat4.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}

	void SetVector(const std::string &name, const VectorData &e) {
		uniform_vec4[name] = e;
	}

	void GetVector(std::map<std::string, VectorData> &m) {
		auto it = uniform_vec4.begin();
		auto ite = uniform_vec4.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}

	size_t GetViewportSize() {
		return viewports.size();
	}

	void ResizeViewportData(int size) {
		viewports.resize(size);
	}

	void GetViewportData(int index, ViewportData *data) {
		*data = viewports[index];
	}

	void SetViewportData(int index, ViewportData *data) {
		viewports[index] = *data;
	}

	void SetRenderTarget(RenderTargetBuffer *p) {
		rendertarget = p;
	}

	RenderTargetBuffer *GetRenderTarget() {
		return rendertarget;
	}
private:
	float width = 0;
	float height = 0;
	RenderTargetBuffer *rendertarget = nullptr;
	std::vector<ViewportData> viewports;
	
	float clearcolor[4];
	int order;
	std::map<std::string, VertexBuffer *> vimap;
	std::map<std::string, MatrixData>  uniform_mat4;
	std::map<std::string, VectorData>  uniform_vec4;
};

class VertexBuffer : public BaseBuffer {
public:
	typedef enum {
		TRIANGLES = 0,
		LINES,
		LINE_STRIP,
		POINTS,
		PATCHES,
	} Topologie;
	enum {
		Vertex = 0,
		Normal,
		Uv,
		Color,
		Max,
	};
	struct Format {
		float pos  [3] ;
		float nor  [3] ;
		float uv   [2] ;
		float color[4] ;
	};
	VertexBuffer() {
		SetType(BUFFER_TYPE_VERTEX);
		topologie = TRIANGLES;
	}

	virtual ~VertexBuffer() {
	}

	void SetMatrix(const std::string &name, const MatrixData &e) {
		uniform_mat4[name] = e;
	}

	void GetMatrix(std::map<std::string, MatrixData> &m) {
		auto it = uniform_mat4.begin();
		auto ite = uniform_mat4.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}

	void SetVector(const std::string &name, const VectorData &e) {
		uniform_vec4[name] = e;
	}

	void GetVector(std::map<std::string, VectorData> &m) {
		auto it = uniform_vec4.begin();
		auto ite = uniform_vec4.end();
		while (it != ite) {
			m[it->first] = it->second;
			it++;
		}
	}

	void Create(int size) {
		buffer.resize(size);
		memset(&buffer[0], 0, sizeof(Format) * size);
	}

	void SetPos(int index, float x, float y, float z) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].pos[0] = x;
		buffer[index].pos[1] = y;
		buffer[index].pos[2] = z;
	}
	void SetNormal(int index, float x, float y, float z) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].nor[0] = x;
		buffer[index].nor[1] = y;
		buffer[index].nor[2] = z;
	}

	void SetUv(int index, float u, float v) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].uv[0] = u;
		buffer[index].uv[1] = v;
	}

	void SetColor(int index, float x, float y, float z, float w) {
		if (buffer.size() <= index) {
			printf("%s : %s : Not alloc index=%d\n", __FUNCTION__, GetName().c_str(), index);
			return;
		}
		buffer[index].color[0] = x;
		buffer[index].color[1] = y;
		buffer[index].color[2] = z;
		buffer[index].color[3] = w;
	}

	VertexBuffer::Format *GetBuffer() {
		return &buffer[0];
	}

	size_t GetBufferSize() {
		return buffer.size() * sizeof(VertexBuffer::Format);
	}

	size_t GetCount() {
		return buffer.size();
	}

	void SetShaderName(const std::string &name) {
		shader_name0 = name;
	}

	std::string GetShaderName() {
		return shader_name0;
	}

	Topologie GetTopologie() {
		return topologie;
	}

	void SetTopologie(Topologie t) {
		topologie = t;
	}

	void SetTextureFile(const std::string &texname, const std::string &filename) {
		uniform_texture_name[texname] = filename;
	}

	void GetTextureFile(std::map<std::string, std::string> &ref) {
		ref = uniform_texture_name;
	}

private:
	Topologie topologie;
	std::vector<Format> buffer;
	std::map<std::string, MatrixData>  uniform_mat4;
	std::map<std::string, VectorData>  uniform_vec4;
	std::map<std::string, std::string> uniform_texture_name;
	std::map<int, std::string> uniform_texture_index;
	std::string shader_name0;
};

