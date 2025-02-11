#ifndef CULLLIGHTINGCOMMON_H
#define CULLLIGHTINGCOMMON_H

struct Plane
{
    float3 N; // plane normal
    float  d; // distance to origin
};

// Compute a plane from 3 non-collinear points that for a triangle.
// This equation assumes a left-handed(counter-clockwise widing order)
// coordinate system to determine the direction of the plane normal.
Plane ComputePlane(float3 p0, float3 p1, float3 p2)
{
    Plane plane;
    
    float3 v0 = p1 - p0;
    float3 v2 = p2 - p0;
    
    plane.N = normalize(cross(v2, v0)); // left-handed
    
    // Compute the distance to the origin using p0.
    plane.d = dot(plane.N, p0);
    
    return plane;
}

// Four planes of a view frustum (in view space)
// The planes are :
// * Left,
// * Right,
// * Top,
// * Bottom.
// The back and//or front planes can be computed from depth values 
// in the light culling compute shader
struct Frustum
{
    // left, right, top, bottom frustum planes.
    Plane Planes[4];
};


// Transfrom functions

cbuffer ScreenToViewParams
{
    float4x4 InverseProjection;
    float2 InvScreenDimensions;
}

float ConvertProjDepthToView(float z)
{
    z = 1.f / (z * InverseProjection._34 + InverseProjection._44);
    return z;
}

float4 ClipToView(float4 clip)
{
    float4 view = mul(clip, InverseProjection);
    
    view /= view.w;
    
    return view;
}

// Convert screen space coordinates to view space
float4 ScreenToView(float4 screen)
{
    // Screen coordinates 2D: [-1, 1] * [-1, 1]
    // (0,0)x------------>
    // y
    // |
    // |
    // |
    // V
    
    // View Space 3D: [-1,1] * [-1, 1] * [0, 1]
    // y
    // |
    // |
    // |
    // V
    // (0,0)x------------>
    
    // Covert to normalized texture coordinates;
    float2 texCoord = screen.xy * InvScreenDimensions;
    
    // Convert to clip space
    float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2 - 1.0, screen.z, screen.w);
    
    // convert to view space
    return ClipToView(clip);
}

// Frustum Culling algorithm
struct Sphere
{
    float3 c; // Center point
    float  r; // Radius
};

// Check to see if a sphere is fully behind (inside the negative halfspace of ) a plane.
bool SphereInsidePlane(Sphere sphere, Plane plane)
{
    return dot(plane.N, sphere.c) - plane.d < -sphere.r;
}

// Chech to see of a light is partially contained within the frustum
bool SphereInsideFrustum(Sphere sphere, Frustum frustum, float zNear, float zFar)
{
    bool result = true;

    // First check depth
    // Note: Here, the view vector points in the -Z axis so the 
    // far depth value will be approaching -infinity.
    if (sphere.c.z + sphere.r < zNear || sphere.c.z - sphere.r > zFar)
    {
        result = false;
    }

    // Then check frustum planes
    for (int i = 0; i < 4 && result; i++)
    {
        if (SphereInsidePlane(sphere, frustum.Planes[i]))
        {
            result = false;
        }
    }

    return result;
}

//
#endif // !CULLLIGHTINGCOMMON_H