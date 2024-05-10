

struct vertexShaderOutput
{
    float32_t4 position : SV_POSITION;
};


struct VertexShaderInput
{
    float32_t4 position : POSITION0;
};


vertexShaderOutput main(VertexShaderInput input)
{
    vertexShaderOutput output;
    output.position = input.position;
    return output;
}