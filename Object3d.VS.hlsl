struct TransformationMatrix
{
    float32_t4x4 WVP;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

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
    output.position = mul(input.position, gTransformationMatrix.WVP);
    return output;
}