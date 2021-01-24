
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

struct VSInput
{
    float4 Position : POSITION;
    float4 Color : COLOR;
};

PSInput VSMain(VSInput Input)
{
    PSInput Result;

    Result.position = Input.Position;
    Result.color = Input.Color;

    return Result;
}

float4 PSMain(PSInput Input) : SV_TARGET
{
    return Input.color;
}
