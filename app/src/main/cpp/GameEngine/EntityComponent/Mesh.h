#pragma once
#include <vector>

#include <glm/glm.hpp>
//#include <vulkan/vulkan.h>

struct Model {
    glm::mat4 model;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 tex;
};

class Mesh{
public:
    Mesh();
    Mesh(/*VkPhysicalDevice newPhysicalDevice, VkDevice newDevice,
         VkQueue transferQueue, VkCommandPool transferCommandPool,*/
         std::vector<Vertex> * vertices, std::vector<uint32_t> * indices,
         int newTexId);

    void setModel(glm::mat4 newModel);
    Model* getModel();

    int getTexId();

    int getVertexCount();
    // getVertexBuffer();

    int getIndexCount();
    //VkBuffer getIndexBuffer();

    void destroyBuffers();

    ~Mesh();

private:
    Model model;
    int texId;
    int vertexCount;
    //VkBuffer vertexBuffer;
    //VkDeviceMemory vertexBufferMemory;

    int indexCount;
    //VkBuffer indexBuffer;
    //VkDeviceMemory indexBufferMemory;

    //VkPhysicalDevice physicalDevice;
    //VkDevice device;

    //void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex> * vertices);
    //void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> * indices);
};