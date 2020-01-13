#include "Win.h"
#include "Buffer.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.hpp"

#pragma warning(disable: 4477)

class Resource {
public:
	Resource() {
	}
	virtual ~Resource() {
	}
};


//
//何を管理するのかというと、
//
// Bufferとかで使う本当のGPUメモリとか
// Samplerコレクション
// Pipelineコレクション
// Shaderコレクション
// DescriptorSetLayoutコレクション
// PipelineLayoutコレクション
// Shaderコレクション
//
//
class ResourceManager {
public:
	struct DeviceMemoryData {
		enum {
			TYPE_LOCAL   = 1 << 0,
			TYPE_HOST    = 1 << 1,
			TYPE_MAX     = 1 << 31,
		};
		vk::DeviceMemory devicememory = nullptr;
		vk::DeviceSize   offset = 0;
	};
	struct Sampler {
		vk::Sampler sampler = nullptr;
	};
	struct Layout {
		vk::DescriptorSetLayout descriptorSetLayout = nullptr;
		vk::PipelineLayout pipelineLayout = nullptr;
	};
	enum {
		MEMORY_TYPE_LOCAL,
		MEMORY_TYPE_HOST,
		MEMORY_TYPE_MAX,
	};
	enum {
		SAMPLER_NEAREST,
		SAMPLER_LINEAR,
		SAMPLER_MAX,
	};
	enum {
		DESC_TYPE_SAMPLER,
		DESC_TYPE_COMBINEDIMAGESAMPLER,
		DESC_TYPE_SAMPLEDIMAGE,
		DESC_TYPE_STORAGEIMAGE,
		DESC_TYPE_UNIFORMTEXELBUFFER,
		DESC_TYPE_STORAGETEXELBUFFER,
		DESC_TYPE_UNIFORMBUFFER,
		DESC_TYPE_STORAGEBUFFER,
		DESC_TYPE_UNIFORMBUFFERDYNAMIC,
		DESC_TYPE_STORAGEBUFFERDYNAMIC,
		DESC_TYPE_INPUTATTACHMENT,
		DESC_TYPE_MAX,
	};
	ResourceManager(
		vk::Device _device, 
		vk::PhysicalDevice _physicalDevice,
		vk::DeviceSize gpusize, 
		vk::DeviceSize desc_pool_size) 
	{
		//ref
		device = _device;
		physicalDevice = _physicalDevice;
		vk::PhysicalDeviceMemoryProperties devicememoryprop = physicalDevice.getMemoryProperties();
		
		//Create Misc Memory
		vk::DeviceSize allocsize = gpusize;
		vk::MemoryPropertyFlags memorypropflags[MEMORY_TYPE_MAX] = {
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		};
		for(int i = 0; i < MEMORY_TYPE_MAX; i++) {
			vk::MemoryPropertyFlags flags = memorypropflags[i];
			vk::MemoryAllocateInfo info = {};
			info.allocationSize = allocsize;
			for (uint32_t i = 0; i < devicememoryprop.memoryTypeCount; i++) {
				if ((devicememoryprop.memoryTypes[i].propertyFlags & flags) == flags) {
					info.memoryTypeIndex = i;
					break;
				}
			}
			vk::DeviceMemory devicememory = nullptr;
			device.allocateMemory(&info, nullptr, &devicememory);
			printf("vkAllocateMemory=%016X\n", devicememory);

			DeviceMemoryData data = {};
			data.devicememory = devicememory;
			data.offset = 0;
			vDeviceMemory[i] = data;
		}
		
		//Create Descripter Pool
		//TODO per type
		{
			const uint32_t countValue = desc_pool_size;
			const uint32_t maxSets = desc_pool_size;

			std::vector<vk::DescriptorPoolSize> vPoolSize;
			vPoolSize.push_back({vk::DescriptorType::eSampler, countValue});
			vPoolSize.push_back({vk::DescriptorType::eCombinedImageSampler, countValue});
			vPoolSize.push_back({vk::DescriptorType::eSampledImage, countValue});
			vPoolSize.push_back({vk::DescriptorType::eStorageImage, countValue});
			vPoolSize.push_back({vk::DescriptorType::eUniformTexelBuffer, countValue});
			vPoolSize.push_back({vk::DescriptorType::eStorageTexelBuffer, countValue});
			vPoolSize.push_back({vk::DescriptorType::eUniformBuffer, countValue});
			vPoolSize.push_back({vk::DescriptorType::eStorageBuffer, countValue});
			vPoolSize.push_back({vk::DescriptorType::eUniformBufferDynamic, countValue});
			vPoolSize.push_back({vk::DescriptorType::eStorageBufferDynamic, countValue});
			vPoolSize.push_back({vk::DescriptorType::eInputAttachment, countValue});

			vk::DescriptorPoolCreateInfo info = {};
			info.poolSizeCount = vPoolSize.size();
			info.pPoolSizes = vPoolSize.data();
			info.maxSets = maxSets;
			device.createDescriptorPool(&info, nullptr, &descPool);
			printf("createDescriptorPool=%016X\n", descPool);
		}
	}

	Sampler GetSampler(uint32_t type) {
		std::string format_name = std::string("Sampler_");
		if(type == SAMPLER_NEAREST) format_name += std::string("_NEAREST_");
		if(type == SAMPLER_LINEAR) format_name += std::string("_LINEAR_");
		auto it = vSampler.find(format_name);
		if(it != vSampler.end()) {
			return it->second;
		}
		vk::Sampler sampler = nullptr;
		vk::SamplerCreateInfo sampleInfo = {};
		if(type == SAMPLER_NEAREST) {
			sampleInfo.magFilter = vk::Filter::eNearest;
			sampleInfo.minFilter = vk::Filter::eNearest;
		}
		if(type == SAMPLER_LINEAR) {
			sampleInfo.magFilter = vk::Filter::eLinear;
			sampleInfo.minFilter = vk::Filter::eLinear;
		}

		sampleInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		sampleInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
		sampleInfo.addressModeV = sampleInfo.addressModeU;
		sampleInfo.addressModeW = sampleInfo.addressModeU;
		sampleInfo.mipLodBias = 0.0f;
		sampleInfo.anisotropyEnable = false;
		sampleInfo.maxAnisotropy = 1.0f;
		sampleInfo.minLod = 0.0f;
		sampleInfo.maxLod = 1.0f;
		sampleInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
		device.createSampler(&sampleInfo, nullptr, &sampler);
		vSampler[format_name] = sampler;
		Sampler sampler = 
		{
			sampler,
		};
		printf("createSampler=%016X\n", sampler);
		return sampler;
	}

	Layout GetLayout(std::vector<uint32_t> vDescFormats) {
		std::string format_name = std::string("DescriptorSetLayout_");
		std::vector<vk::DescriptorSetLayoutBinding> vBinding;
		for(uint32_t i = 0 ; i < vDescFormats.size(); i++) {
			uint32_t data = vDescFormats[i];
			vk::DescriptorType type = vk::DescriptorType::eSampler;
			if(data == DESC_TYPE_SAMPLER)              { format_name += std::string("_SAMPLER_");  type = vk::DescriptorType::eSampler; }
			if(data == DESC_TYPE_COMBINEDIMAGESAMPLER) { format_name += std::string("_COMBINEDIMAGESAMPLER_");  type = vk::DescriptorType::eCombinedImageSampler; }
			if(data == DESC_TYPE_SAMPLEDIMAGE)         { format_name += std::string("_SAMPLEDIMAGE_");  type = vk::DescriptorType::eSampledImage; }
			if(data == DESC_TYPE_STORAGEIMAGE)         { format_name += std::string("_STORAGEIMAGE_");  type = vk::DescriptorType::eStorageImage; }
			if(data == DESC_TYPE_UNIFORMTEXELBUFFER)   { format_name += std::string("_UNIFORMTEXELBUFFER_");  type = vk::DescriptorType::eUniformTexelBuffer; }
			if(data == DESC_TYPE_STORAGETEXELBUFFER)   { format_name += std::string("_STORAGETEXELBUFFER_");  type = vk::DescriptorType::eStorageTexelBuffer; }
			if(data == DESC_TYPE_UNIFORMBUFFER)        { format_name += std::string("_UNIFORMBUFFER_");  type = vk::DescriptorType::eUniformBuffer; }
			if(data == DESC_TYPE_STORAGEBUFFER)        { format_name += std::string("_STORAGEBUFFER_");  type = vk::DescriptorType::eStorageBuffer; }
			if(data == DESC_TYPE_UNIFORMBUFFERDYNAMIC) { format_name += std::string("_UNIFORMBUFFERDYNAMIC_");  type = vk::DescriptorType::eUniformBufferDynamic; }
			if(data == DESC_TYPE_STORAGEBUFFERDYNAMIC) { format_name += std::string("_STORAGEBUFFERDYNAMIC_");  type = vk::DescriptorType::eStorageBufferDynamic; }
			if(data == DESC_TYPE_INPUTATTACHMENT)      { format_name += std::string("_INPUTATTACHMENT_");  type = vk::DescriptorType::eInputAttachment; }
			vBinding.push_back({i, type, 1, vk::ShaderStageFlagBits::eAllGraphics, nullptr});
		}
		printf("%s : format_name=%s\n", __FUNCTION__, format_name.c_str());
		
		//find cache
		auto it = vLayout.find(format_name);
		if(it != vLayout.end()) {
			return it->second;
		}
		
		//create
		vk::DescriptorSetLayout descSetLayout = nullptr;
		vk::PipelineLayout pipelineLayout = nullptr;
		{
			vk::DescriptorSetLayoutCreateInfo info = {};
			info.pBindings = vBinding.data();
			info.bindingCount = vBinding.size();
			device.createDescriptorSetLayout(&info, nullptr, &descSetLayout);
			printf("createDescriptorSetLayout=%016X\n", descSetLayout);
		}
		
		//create pipelineLayout
		if(descSetLayout) {
			std::vector<vk::DescriptorSetLayout> vTemp = {
				descSetLayout,
			};
			vk::PipelineLayoutCreateInfo info = {};
			info.pNext = nullptr;
			info.setLayoutCount = vTemp.size();
			info.pSetLayouts = vTemp.data();
			device.createPipelineLayout(&info, nullptr, &pipelineLayout);
			printf("createPipelineLayout=%016X\n", pipelineLayout);
		} else {
			printf("ERROR : %s : failed create descriptorSetLayout\n", __FUNCTION__);
		}
		
		//add cache
		Layout ret = {
			descSetLayout,
			pipelineLayout,
		};
		vLayout[format_name] = ret;
		return ret;
	}

	~ResourceManager() {
	}
private:
	//危険なら後で何とかする
	vk::Device device = nullptr; 
	vk::PhysicalDevice physicalDevice = nullptr;

	vk::DescriptorPool descPool = nullptr;
	vk::PhysicalDeviceMemoryProperties devicememoryprop;
	std::map<std::string, Layout> vLayout;
	std::map<std::string, vk::Sampler> vSampler;
	std::map<uint32_t, DeviceMemoryData> vDeviceMemory;
};


//ここからはk
class Vertex : public Resource {
	int vertex_count = 0;
public:
	Vertex(ResourceManager *manager, void *data, size_t size, size_t stride_size) {

	}
	int GetCount() {
		return vertex_count;
	}
};



class Shader : public Resource {
public:
	~Shader() {
	}

	
	// https://github.com/google/skia/blob/master/src/gpu/vk/GrVkPipelineState.cpp
	//
	//めんどくさいので全部こいつに閉じ込めたい気がする
	// シェーダーには何を入力/出力するのかが書いてあるけど、それはあくまでもGLSLとかそっちレベルの話
	// GPU側には、その識別子自体の入出力をするためのリソース確保(descripterSetsはPoolから作られる)
	// レイアウトとかそういうのってシェーダー側と結合度がめちゃんこ高いので、
	// Shader内部のクラスとして管理した方が絶対に良いと思ってる。
	// deviceに必要なものは、descripterPool
	//
	//
	Shader(ResourceManager *manager, const char *vsfile, const char *fsfile, uint32_t state) { 
	}

};

class Texture : public Resource {
public:
	~Texture() {
	}
	Texture(ResourceManager *manager, int w, int h, int format = TextureBuffer::FORMAT_RGBA8, const void *data = nullptr) {
	}
};

class RenderTarget : public Resource {
	std::vector<Texture *> textures;
	int Width = 0;
	int Height = 0;
	//AND framebuffer
public:
	Texture *GetTexture(int index = 0) {
		return textures[index];
	}

	~RenderTarget() {
	}

	RenderTarget(ResourceManager *manager, int w, int h, int count) {
	}
};

class Renderer {
	vk::ApplicationInfo appInfo;
	vk::InstanceCreateInfo instanceInfo;
	vk::Instance instance;
	std::vector<vk::PhysicalDevice> physicalDevices;
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties;
	vk::DeviceQueueCreateInfo queueCreateInfo;
	vk::Device device;
	vk::Queue  queue ;
	vk::SurfaceKHR surface;
	std::vector<vk::SurfaceFormatKHR> surfaceFormats;
	std::vector<vk::PresentModeKHR> presentModes;
	vk::SurfaceCapabilitiesKHR surfCaps;
	vk::SwapchainKHR swapChain ;
	std::vector<vk::Image> swapImages;
	
	std::map<std::string, BaseBuffer *> buffermapper;
	template <typename T>
	T *GetBuffer(const std::string &name) {
		T *ret = (T *)FindBuffer(name);
		if (!ret) {
			ret = new T;
			ret->SetName(name);
			buffermapper[name] = ret;
		}
		return ret;
	}
	std::map<std::string, Vertex *> cache_vertex;
	std::map<std::string, Shader *> cache_shader;
	std::map<std::string, Texture *> cache_texture;
	std::map<std::string, RenderTarget *> cache_rendertarget;

	ResourceManager *manager = nullptr;
public:

	ViewBuffer *GetViewBuffer(const std::string &name) {
		return GetBuffer<ViewBuffer>(name);
	}

	VertexBuffer *GetVertexBuffer(const std::string &name) {
		return GetBuffer<VertexBuffer>(name);
	}

	TextureBuffer *GetTextureBuffer(const std::string &name) {
		return GetBuffer<TextureBuffer>(name);
	}

	RenderTargetBuffer *GetRenderTargetBuffer(const std::string &name) {
		return GetBuffer<RenderTargetBuffer>(name);
	}

	BaseBuffer *FindBuffer(const std::string &name) {
		auto it = buffermapper.find(name);
		if (it != buffermapper.end()) {
			return it->second;
		}
		return 0;
	}
	void MarkUpdate(int type) {
		auto it = buffermapper.begin();
		auto ite = buffermapper.end();
		for (; it != ite; it++) {
			auto *buf = it->second;
			if (buf->GetType() == type) {
				buf->MarkUpdate();
			}
		}
	}
	//Update
	//頂点バッファとかそういうの
	void Update() {
		//レンダリングしようと試みているデータを更新する
		//Buffer側では、レンダリングするデータをシステムメモリに置いてあるけど
		//VKでは、
		//Bufferハンドルを作成 -> バインドするメモリ領域を作成、ないし選択 -> 選択してBind -> 必要に応じてMapしてデータ転送
		//システム側からローカル側へデータを転送するので、システム側とGPU側でデータが見える領域を作成して、TRANSFERを行う
		
		//レンダリングしようとして先を取ってきて存在しないなら、
		//Images + Depthでrenderpass + framebufferを作成。そんだけ。シンプル
		//
	}
	
	//Render
	//画面にでるフレームバッファとかそういうのの実instance作成と更新
	void Render(std::string view_name) {
		ViewBuffer *view = (ViewBuffer *)buffermapper[view_name];
		if(!view) return;
		int rtvCount = 1;
		auto *rt = view->GetRenderTarget();
		RenderTarget *curr_rt = NULL;
		if (rt) {
			rtvCount = rt->GetCount();
			std::string rt_name = rt->GetName();
			if (cache_rendertarget.find(rt_name) == cache_rendertarget.end()) {
				auto *rendertarget = new RenderTarget(nullptr, rt->GetWidth(), rt->GetHeight(), rtvCount);
				for (int i = 0 ; i < rtvCount; i++) {
					Texture *texture = rendertarget->GetTexture(i);
					TextureBuffer *color_tex = rt->GetTexture(i, false);
					cache_texture[color_tex->GetName()] = texture;
				}
				cache_rendertarget[rt_name] = rendertarget;
			}
			curr_rt = cache_rendertarget[rt_name];
		}

		//レンダーターゲット指定されているならフレームバッファ設定
		if(curr_rt) {
			//glBindFramebuffer(GL_FRAMEBUFFER, curr_rt->Get());
			//std::vector<GLenum> DrawBuffers;
			//for(int i = 0 ; i < curr_rt->GetCount(); i++) {
			//	DrawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
			//}
			//glDrawBuffers(curr_rt->GetCount(), &DrawBuffers[0]);
			//int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			//if(status != GL_FRAMEBUFFER_COMPLETE ) {
			//	printf("ERROR GL_FRAMEBUFFER_COMPLETE Failed : status=%08X\n", status);
			//}
		}

		//Viewports
		float clear_color[4] = {};
		view->GetClearColor(clear_color);
		//glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glViewport(0, 0, view->GetWidth(), view->GetHeight());
		//glScissor(0, 0, view->GetWidth(), view->GetHeight());
        //
		//glEnable(GL_TEXTURE_2D);
		//glEnable(GL_DEPTH_TEST);

		//per viewports
		bool is_multi_rendering = false;
		size_t viewport_count = view->GetViewportSize();
		if(viewport_count > 0) {
			is_multi_rendering = true;
		} else {
			viewport_count = 1;
		}
		std::vector<VertexBuffer *> temp_vertex_buffer;
		view->GetVertexBuffer(temp_vertex_buffer);
		for(size_t i = 0 ; i < viewport_count; i++) {
			
			//ビューポート、シザーを取得して設定
			ViewBuffer::ViewportData viewport_data;
			MatrixData matrix_proj  ;
			MatrixData matrix_view  ;
			MatrixData matrix_world ;
			if(is_multi_rendering) {
				view->GetViewportData(i, &viewport_data);
				matrix_proj  = viewport_data.proj;
				matrix_view  = viewport_data.view;
				matrix_world = viewport_data.world;
			} else {
				viewport_data.x = 0.0;
				viewport_data.y = 0.0;
				viewport_data.w = view->GetWidth();
				viewport_data.h = view->GetHeight();
				matrix_proj  = view->GetMatrix("proj");
				matrix_view  = view->GetMatrix("view");
				matrix_world = view->GetMatrix("world");
				
				
			}
			
			//Set Viewport
			//glViewport(viewport_data.x, viewport_data.y, viewport_data.w, viewport_data.h);
			//glScissor(viewport_data.x, viewport_data.y, viewport_data.w, viewport_data.h);
			
			for (auto vertex : temp_vertex_buffer) {
				auto buf = vertex;

				auto topo = buf->GetTopologie();


				std::map<std::string, MatrixData> buf_matrix;
				std::map<std::string, VectorData> buf_vectors;

				buf->GetMatrix(buf_matrix);
				buf->GetVector(buf_vectors);
				std::string shader_name = buf->GetShaderName();

				//Create Pipeline and layout
				if (!shader_name.empty()) {
					auto it = cache_shader.find(shader_name);
					if (it == cache_shader.end()) {
						//Shader *shader = new Shader(
						//	nullptr,
						//	(shader_name + std::string(".vsh")).c_str(),
						//	(shader_name + std::string(".fsh")).c_str());
						//auto it = cache_shader.find(shader_name);
						//if (it != cache_shader.end()) {
						//	delete it->second;
						//}
						//cache_shader[shader_name] = shader;
					}
				}

				//シェーダー設定
				auto shader_object = cache_shader[shader_name];
				if (!shader_object) {
					printf("シェーダーが設定されていない : %s\n", shader_name.c_str());
					
					buf = (VertexBuffer *)buf->GetChild();
					continue;
				}

				//auto prog = shader_object->Get();
				////glUseProgram(prog);


				{
					std::map<std::string, MatrixData> matrix;
					view->GetMatrix(matrix);
					for(auto &m : matrix) {
						auto name = m.first.c_str();
						auto data = m.second;
						//GLuint loc = glGetUniformLocation(prog, name);
						//glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)&data);
					}
				}
				{
					std::map<std::string, VectorData> vectors;
					view->GetVector(vectors);
					for(auto &m : vectors) {
						auto name = m.first.c_str();
						auto data = m.second;
						//GLuint loc = glGetUniformLocation(prog, name);
						//glUniform4fv(loc, 1, (const GLfloat *)&data);
					}
				}

				{
					//GLuint loc = glGetUniformLocation(prog, "proj");
					//glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)&matrix_proj);
				}
				{
					//GLuint loc = glGetUniformLocation(prog, "view");
					//glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)&matrix_view);
				}
				{
					//GLuint loc = glGetUniformLocation(prog, "world");
					//glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)&matrix_world);
				}

				//VertexBuffer側のuniform設定
				for(auto &m : buf_matrix) {
					auto name = m.first.c_str();
					auto data = m.second;
					//GLuint loc = glGetUniformLocation(prog, name);
					//glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)&data);
				}
				for(auto &m : buf_vectors) {
					auto name = m.first.c_str();
					auto data = m.second;
					//GLuint loc = glGetUniformLocation(prog, name);
					//glUniform4fv(loc, 1, (const GLfloat *)&data);
				}
				
				int index = 0;
				while(buf) {
					//頂点バッファ設定
					auto vertex_object = cache_vertex[buf->GetName()];
					if (!vertex_object) {
						auto str = vertex->GetName();
						printf("頂点バッファがありません : %s\n", str.c_str());

						buf = (VertexBuffer *)buf->GetChild();
						continue;
					}
					//glBindVertexArray(vertex_object->Get());

					//テクスチャ設定
					std::map<std::string, std::string> uniform_texture_name;
					buf->GetTextureFile(uniform_texture_name);
					int index = 0;
					for(auto &tex : uniform_texture_name) {
						auto name = tex.first.c_str();
						auto filename = tex.second.c_str();

						//printf("name=%s, filename=%s\n", name, filename);

						auto texture_object = cache_texture[filename];
						if(texture_object) {
							//GLuint loc = glGetUniformLocation(prog, name);
							//glActiveTexture(GL_TEXTURE0 + index);
							//glBindTexture(GL_TEXTURE_2D, texture_object->Get());
							//glUniform1i(loc, index);
						} else {
							auto nulltexture = cache_texture["nulltexture"];
							if(nulltexture) {
								//GLuint loc = glGetUniformLocation(prog, name);
								//glActiveTexture(GL_TEXTURE0 + index);
								//glBindTexture(GL_TEXTURE_2D, nulltexture->Get());
								//glUniform1i(loc, index);
								////printf("loc=%d\, index=%d, texhandle = %llX\n", loc, index, nulltexture->Get());
							}
						}
						index++;
					}

					//printf("topo = %d\n", topo);
					//if(topo == VertexBuffer::TRIANGLES) {
					//	glDrawArrays(GL_TRIANGLES, 0, vertex_object->GetCount());
					//}
					//
					//if(topo == VertexBuffer::LINES) {
					//	glDrawArrays(GL_LINES, 0, vertex_object->GetCount());
					//}
					//if(topo == VertexBuffer::LINE_STRIP) {
					//	glDrawArrays(GL_LINE_STRIP, 0, vertex_object->GetCount());
					//}
                    //
					//if(topo == VertexBuffer::POINTS) {
					//	glDrawArrays(GL_POINTS, 0, vertex_object->GetCount());
					//}

					//子も表示
					buf = (VertexBuffer *)buf->GetChild();
				}
				//printf("buf index=%d, buf=%llX\n", index++, buf);
				//glUseProgram(0);
				
			}

			//glBindVertexArray(0);
		}
		//フレームバッファバインド終わり
		if(curr_rt) {
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
	
	Renderer(const char *appName, const char *engineName, HWND hWnd, HINSTANCE hInstance, uint32_t width, uint32_t height) {

		///vulkan
		std::vector<const char *> layerNames = {
			"VK_LAYER_LUNARG_standard_validation",
		};
		std::vector<const char *> extSwap = { "VK_KHR_swapchain" };
		std::vector<const char *> extNames = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		};

		//instance作成
		appInfo = vk::ApplicationInfo(appName, 1, engineName, 1, VK_MAKE_VERSION(1,0,30));
		instanceInfo = vk::InstanceCreateInfo({}, &appInfo, layerNames.size(), layerNames.data(), extNames.size(), extNames.data());
		vk::createInstance(&instanceInfo, nullptr, &instance);

		//物理デバイス作成
		physicalDevices = instance.enumeratePhysicalDevices();
		if(physicalDevices.empty()) {
			printf("ぐらぼがしんでる\n");
		}
		vk::PhysicalDevice physicalDevice = physicalDevices[0];
		queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
		uint32_t graphicsQueueFamilyIndex = 0;
		for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
			if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
				graphicsQueueFamilyIndex = i;
				break;
			}
		}

		//create device and queue
		const float defaultQueuePriority(0.0f);
		queueCreateInfo = vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), graphicsQueueFamilyIndex, 1, &defaultQueuePriority);
		std::vector<const char *> deviceExtensions;
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.pEnabledFeatures = nullptr;
		if (deviceExtensions.size() > 0) {
			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}
		device = physicalDevice.createDevice(deviceCreateInfo);
		queue = device.getQueue(graphicsQueueFamilyIndex, 0);

		
		//Create Swapchain
		vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.hinstance = (HINSTANCE)hInstance;
		surfaceCreateInfo.hwnd = (HWND)hWnd;
		surface = instance.createWin32SurfaceKHR(surfaceCreateInfo);
		surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
		surfCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		printf("device=%llX\n", device);
		printf("queue=%llX\n", queue);
		printf("surface=%llX\n", surface);
		printf("surfaceFormats.size()=%llX\n", surfaceFormats.size());
		presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

		std::vector<vk::Bool32> supportsPresent(queueFamilyProperties.size());
		for(size_t i = 0; i < queueFamilyProperties.size(); i++) {
			supportsPresent[i] = physicalDevice.getSurfaceSupportKHR(i, surface);
		}

		
		vk::SwapchainCreateInfoKHR swapchainCI;
		swapchainCI.surface = surface;
		swapchainCI.minImageCount = surfCaps.minImageCount;
		swapchainCI.imageFormat = vk::Format::eB8G8R8A8Unorm;;
		swapchainCI.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
		swapchainCI.imageExtent.width = width;
		swapchainCI.imageExtent.height = height;
		swapchainCI.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = vk::SharingMode::eExclusive;
		swapchainCI.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices = NULL;
		swapchainCI.presentMode = vk::PresentModeKHR::eFifo;
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapchainCI.oldSwapchain = vk::SwapchainKHR();

		swapChain = device.createSwapchainKHR(swapchainCI);

		//Create images
		swapImages = device.getSwapchainImagesKHR(swapChain);

		for(auto &img : swapImages) {
			printf("images=%llX\n", img);
		}
		
		
		//Create Resource Manager
		manager = new ResourceManager(device, physicalDevice, 1024 * 1024 * 512, 32);
		
		//
		// TEST
		//
		//1, 4 sampler
		std::vector<uint32_t> vBasicDescFormats = {
			ResourceManager::DESC_TYPE_UNIFORMBUFFER,
			ResourceManager::DESC_TYPE_COMBINEDIMAGESAMPLER,
			ResourceManager::DESC_TYPE_COMBINEDIMAGESAMPLER,
			ResourceManager::DESC_TYPE_COMBINEDIMAGESAMPLER,
			ResourceManager::DESC_TYPE_COMBINEDIMAGESAMPLER,
		};
		auto descriptorSetLayout = manager->GetLayout(vBasicDescFormats);
		auto sampler_nearest = manager->GetSampler(ResourceManager::SAMPLER_NEAREST);
		auto sampler_linear = manager->GetSampler(ResourceManager::SAMPLER_LINEAR);
	}
};


class App : public Window {
	std::string appname = "";
	int Width = -1;
	int Height = -1;
public:
	App(const char *name, int w = 1280, int h = 720) : Window(name, w, h) {
		appname = std::string(name);
		Width = w;
		Height = h;
		Window(name, w, h);
	}

	bool Init() {
		std::string enginename = appname + std::string("engine");
		auto renderer = new Renderer(appname.c_str(), enginename.c_str(), GetWindowHandle(), GetInstance(), GetWidth(), GetHeight());
		

		return true;
	}

	void Term() {
	}
	void Update() {
	}
	
};

int main() {
	const char *appName = "vkhpptest.cpp";
	App *app = new App(appName);
	if(app) {
		app->Start();
	}
	return 0;
}
