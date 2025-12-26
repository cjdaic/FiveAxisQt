#include "LaserParamCalculator.h"
#include <cmath>
#include <iostream>
#include <fstream>

// ** Include OpenMesh headers **
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

// Define a mesh with default traits
typedef OpenMesh::TriMesh_ArrayKernelT<>  Mesh;

// Helper: compute area of a face (assuming triangular face)
static double faceArea(const Mesh& mesh, Mesh::FaceHandle fh) {
    // Get vertices of the face
    OpenMesh::FVIter fv_it = mesh.fv_iter(fh);
    Mesh::Point p0 = mesh.point(*fv_it);  ++fv_it;
    Mesh::Point p1 = mesh.point(*fv_it);  ++fv_it;
    Mesh::Point p2 = mesh.point(*fv_it);
    // Compute area = 0.5 * |(p1-p0) x (p2-p0)|
    Mesh::Point v1 = p1 - p0;
    Mesh::Point v2 = p2 - p0;
    Mesh::Point cross = Mesh::Point(v1[1] * v2[2] - v1[2] * v2[1],
        v1[2] * v2[0] - v1[0] * v2[2],
        v1[0] * v2[1] - v1[1] * v2[0]);
    double area = 0.5 * std::sqrt(cross.sqrnorm()); // sqrnorm gives squared length
    return area;
}

bool computeLaserParameters(const std::string& objFile,
    const LaserParams& params,
    std::vector<VertexLaserParams>& vertexParamsOut)
{
    Mesh mesh;
    // Request vertex normals from OpenMesh
    mesh.request_face_normals();
    mesh.request_vertex_normals();
    // Load the mesh from the OBJ file
    if (!OpenMesh::IO::read_mesh(mesh, objFile)) {
        std::cerr << "Error: Cannot read mesh from " << objFile << std::endl;
        return false;
    }
    // Ensure the mesh has face normals for vertex normal computation
    mesh.update_normals();

    size_t nV = mesh.n_vertices();
    vertexParamsOut.clear();
    vertexParamsOut.resize(nV);

    // Determine pulse frequency to use (see approach above)
    double f_selected = params.maxFreq;
    double v_base = (1.0 - params.overlap) * params.spotSize * f_selected;

    // We will iterate to adjust f if needed based on power limits.
    // First, compute worst-case required power at f_max (initial f_selected).
    double worstRequiredPower = 0.0;
    // Also store cos(theta) and curvature for each vertex for reuse.
    std::vector<double> cosTheta(nV);
    std::vector<double> curvature(nV);

    // Precompute curvatures and cosines
    // (We can do this first for all vertices; then adjust f and compute P.)
    for (Mesh::VertexIter vit = mesh.vertices_begin(); vit != mesh.vertices_end(); ++vit) {
        Mesh::VertexHandle vh = *vit;
        int i = vh.idx();
        // Compute normal incidence cosine
        Mesh::Normal vnorm = mesh.normal(vh);
        cosTheta[i] = vnorm[2]; // dot with (0,0,1), assuming normals are unit length.
        // If normal is not unit or not computed, ensure to normalize or compute manually.

        // Compute discrete mean curvature at vertex i:
        // We use the cotan formula (5).
        double area_sum = 0.0;
        Mesh::Point laplace(0.0, 0.0, 0.0);
        // Sum areas of adjacent faces
        for (Mesh::VFIter vf_it = mesh.vf_iter(vh); vf_it.is_valid(); ++vf_it) {
            area_sum += faceArea(mesh, *vf_it);
        }
        double A_i = area_sum / 3.0;  // barycentric area for vertex

        // Sum cotan weights * (p_j - p_i) for each neighbor j
        for (Mesh::VVIter vv_it = mesh.vv_iter(vh); vv_it.is_valid(); ++vv_it) {
            Mesh::VertexHandle jh = *vv_it;
            // Find the two faces sharing edge (i, j)
            Mesh::HalfedgeHandle hij = mesh.find_halfedge(vh, jh);
            if (!hij.is_valid()) continue; // no halfedge (should not happen if mesh is manifold)
            Mesh::HalfedgeHandle hji = mesh.opposite_halfedge_handle(hij);
            // Get face handles (one or both could be invalid if on boundary)
            Mesh::FaceHandle f1 = mesh.face_handle(hij);
            Mesh::FaceHandle f2 = mesh.face_handle(hji);
            double cot_alpha = 0.0, cot_beta = 0.0;
            if (f1.is_valid()) {
                // face f1 has vertices: vh, jh, and another kh
                Mesh::VertexHandle kv;
                // Iterate vertices of f1 to find the one that's not i or j
                for (Mesh::FVIter fv_it = mesh.fv_iter(f1); fv_it.is_valid(); ++fv_it) {
                    if (*fv_it != vh && *fv_it != jh) {
                        kv = *fv_it;
                        break;
                    }
                }
                // Compute cot(alpha) at vertex k (opposite edge i-j in f1)
                Mesh::Point Pi = mesh.point(vh);
                Mesh::Point Pj = mesh.point(jh);
                Mesh::Point Pk = mesh.point(kv);
                Mesh::Point u = Pi - Pk;
                Mesh::Point v = Pj - Pk;
                // cot(alpha) = (u¡¤v) / |u x v|
                Mesh::Point uxv = Mesh::Point(u[1] * v[2] - u[2] * v[1],
                    u[2] * v[0] - u[0] * v[2],
                    u[0] * v[1] - u[1] * v[0]);
                double dotuv = u | v; // OpenMesh overloads '|' for dot product
                double sinArea = std::sqrt(uxv.sqrnorm());
                if (sinArea < 1e-9) {
                    cot_alpha = 0.0;
                }
                else {
                    cot_alpha = dotuv / sinArea;
                }
            }
            if (f2.is_valid()) {
                // face f2 has vertices: vh, jh, and another kh2
                Mesh::VertexHandle kv2;
                for (Mesh::FVIter fv_it = mesh.fv_iter(f2); fv_it.is_valid(); ++fv_it) {
                    if (*fv_it != vh && *fv_it != jh) {
                        kv2 = *fv_it;
                        break;
                    }
                }
                Mesh::Point Pi = mesh.point(vh);
                Mesh::Point Pj = mesh.point(jh);
                Mesh::Point Pk2 = mesh.point(kv2);
                Mesh::Point u2 = Pi - Pk2;
                Mesh::Point v2 = Pj - Pk2;
                Mesh::Point u2xv2 = Mesh::Point(u2[1] * v2[2] - u2[2] * v2[1],
                    u2[2] * v2[0] - u2[0] * v2[2],
                    u2[0] * v2[1] - u2[1] * v2[0]);
                double dotu2v2 = u2 | v2;
                double sinArea2 = std::sqrt(u2xv2.sqrnorm());
                if (sinArea2 < 1e-9) {
                    cot_beta = 0.0;
                }
                else {
                    cot_beta = dotu2v2 / sinArea2;
                }
            }
            // (p_j - p_i)
            Mesh::Point diff = mesh.point(jh) - mesh.point(vh);
            double weight = cot_alpha + cot_beta;
            laplace += weight * diff;
        }
        // Now laplace = sum (cot¦Á+cot¦Â)*(pj - pi). We get mean curvature normal = (1/(2A_i)) * laplace.
        Mesh::Point meanCurvVec = (1.0 / (2.0 * A_i)) * laplace;
        // Curvature magnitude (approx 2H) 
        double H_mag = 0.5 * std::sqrt(meanCurvVec.sqrnorm());
        curvature[i] = H_mag;

        // For initial worst-case power estimate, assume f_selected (initially maxFreq) and base speed:
        double cosVal = cosTheta[i];
        if (cosVal < 1e-6) cosVal = 1e-6; // avoid division by zero (steep angle)
        double E_eff = params.targetEnergy / cosVal * (1.0 + params.curvWeight * curvature[i]);
        double requiredP = E_eff * v_base * params.hatchSpacing;
        if (requiredP > worstRequiredPower) {
            worstRequiredPower = requiredP;
        }
    }

    // Adjust global frequency if needed to keep worstRequiredPower <= maxPower
    if (worstRequiredPower > params.maxPower) {
        // Compute recommended frequency based on worst-case
        double f_limit = params.maxPower / (worstRequiredPower / f_selected);
        // worstRequiredPower was computed as E_eff * v_base * h with v_base = (1-OL)*D*f_selected.
        // Actually, we can derive directly: f_limit = P_max / (E_eff_max * h * (1-OL) * D).
        // For simplicity, using proportion: required P scales linearly with f, so:
        // f_limit = f_selected * (params.maxPower / worstRequiredPower).
        f_limit = f_selected * (params.maxPower / worstRequiredPower);
        if (f_limit < params.minFreq) {
            f_selected = params.minFreq;
        }
        else if (f_limit < f_selected) {
            f_selected = f_limit;
        }
        // Recompute base speed with new f_selected
        v_base = (1.0 - params.overlap) * params.spotSize * f_selected;
    }

    // Now compute final per-vertex parameters with chosen f_selected and v_base
    for (Mesh::VertexIter vit = mesh.vertices_begin(); vit != mesh.vertices_end(); ++vit) {
        Mesh::VertexHandle vh = *vit;
        int i = vh.idx();
        VertexLaserParams vp;
        vp.f = f_selected;
        double cosVal = cosTheta[i];
        if (cosVal < 1e-6) cosVal = 1e-6;
        double E_eff = params.targetEnergy / cosVal * (1.0 + params.curvWeight * curvature[i]);
        // Required power for base speed
        double P_req = E_eff * v_base * params.hatchSpacing;
        // Clamp power to [minPower, maxPower]
        if (P_req > params.maxPower) {
            vp.P = params.maxPower;
        }
        else if (P_req < params.minPower) {
            vp.P = params.minPower;
        }
        else {
            vp.P = P_req;
        }
        // Determine speed
        if (vp.P < P_req) {
            // vp.P was maxPower but that is lower than needed, so processing needs slower speed
            // Calculate speed to satisfy E_eff with P_max
            vp.v = (vp.P) / (E_eff * params.hatchSpacing);
        }
        else if (vp.P > P_req) {
            // vp.P was raised to minPower (higher than needed). Optionally increase speed.
            // To avoid underlap, we won't exceed base v, just keep base speed.
            vp.v = v_base;
        }
        else {
            // P not clamped
            vp.v = v_base;
        }
        vertexParamsOut[i] = vp;
    }

    return true;
}