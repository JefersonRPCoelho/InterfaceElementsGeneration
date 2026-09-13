//
// Created by Jeferson Coelho on 7/4/26.
//

#include "InterfaceElementsWrapper.h"

// bindings.cpp
// Python bindings for the InterfaceElementsGeneration library using pybind11.
//
// Build with:
//   c++ -O3 -Wall -shared -std=c++17 -fPIC \
//       $(python3 -m pybind11 --includes) \
//       bindings.cpp CHE.cpp IBHM.cpp CHEInterfaceElementBuilder.cpp IBHMInterfaceElementBuilder.cpp \
//       -o interface_elements$(python3-config --extension-suffix)

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

#include <memory>
#include <optional>
#include <vector>

#include "core/TopologyDataStructures/CHE.h"
#include "core/TopologyDataStructures/IBHM.h"
#include "core/InterfaceElementsBuilder/CHEInterfaceElementBuilder.h"
#include "core/InterfaceElementsBuilder/IBHMInterfaceElementBuilder.h"

namespace py = pybind11;

PYBIND11_MODULE(interface_elements, m)
{
    m.doc() = "Python bindings for the InterfaceElementsGeneration library.";

    // ──────────────────────────────────────────────
    // CHE
    // ──────────────────────────────────────────────
    py::class_<CHE, std::shared_ptr<CHE>> pyCHE(m, "CHE",
        "Compact Half-Edge data structure for uniform meshes.");

    // Expose the OPPOSITE sentinel values as class-level attributes.
    // In C++ these are unsigned int with values static_cast<unsigned int>(-1) and (-2).
    pyCHE.attr("BORDER")    = static_cast<unsigned int>(CHE::BORDER);
    pyCHE.attr("COLLAPSED") = static_cast<unsigned int>(CHE::COLLAPSED);

    pyCHE
        // Constructor from vectors (most natural for Python).
        .def(py::init<const std::vector<double> &, const std::vector<unsigned int> &,
                       unsigned int, unsigned int>(),
             py::arg("coordinates"), py::arg("elements_list"),
             py::arg("number_vertices_by_element"), py::arg("number_coordinates"),
             "Construct a CHE from coordinate and element vectors.")

        // --- query methods ---
        .def("he_element", &CHE::heElement, py::arg("half_edge"),
             "Return the element index that contains the given half-edge.")
        .def("he_next", &CHE::heNext, py::arg("half_edge"),
             "Return the next half-edge in the element orientation.")
        .def("he_previous", &CHE::hePrevious, py::arg("half_edge"),
             "Return the previous half-edge in the element orientation.")
        .def("he_vertex_index", &CHE::heVertexIndex, py::arg("half_edge"),
             "Return the vertex index attached to the given half-edge.")
        .def("he_opposite", &CHE::heOpposite, py::arg("half_edge"),
             "Return the opposite half-edge, or CHE.BORDER / CHE.COLLAPSED.")
        .def("is_border", &CHE::isBorder, py::arg("half_edge"),
             "Check whether the vertex at the given half-edge lies on the mesh border.")
        .def("next_available_he", &CHE::nextAvailableHE,
             "Return the next available half-edge index for new elements.")

        // --- size queries ---
        .def("number_points", &CHE::numberPoints,
             "Return the number of valid points in the mesh.")
        .def("number_of_elements", &CHE::numberOfElements,
             "Return the number of valid elements in the mesh.")
        .def("number_reserved_elements", &CHE::numberReservedElements,
             "Return the number of reserved element slots.")
        .def("number_coordinates_by_vertex", &CHE::numberCoordinatesByVertex,
             "Return the number of coordinate components per vertex (e.g. 2 for 2D).")
        .def("number_vertex_by_element", &CHE::numberVertexByElement,
             "Return the number of vertices per element (e.g. 3 for triangles).")

        // --- accessors returning data ---
        .def("elements_list", &CHE::elementsList, py::return_value_policy::reference_internal,
             "Return a read-only reference to the elements list.")
        .def("coordinates", &CHE::coordinates, py::return_value_policy::reference_internal,
             "Return a reference to the coordinates vector.")

        // --- coordinate helpers (output-pointer API wrapped for Python) ---
        .def("coordinates_from_half_edge",
             [](const CHE &self, unsigned int halfEdge) {
                 const unsigned int dim = self.numberCoordinatesByVertex();
                 std::vector<double> coords(dim);
                 self.coordinatesFromHalfEge(halfEdge, coords.data());
                 return coords;
             },
             py::arg("half_edge"),
             "Return the coordinates of the vertex attached to the given half-edge.")
        .def("coordinates_from_index",
             [](const CHE &self, unsigned int index) {
                 const unsigned int dim = self.numberCoordinatesByVertex();
                 std::vector<double> coords(dim);
                 self.coordinatesFromIndex(index, coords.data());
                 return coords;
             },
             py::arg("index"),
             "Return the coordinates of the vertex at the given index.")

        // --- mutation methods ---
        .def("add_point",
             [](CHE &self, std::optional<std::vector<double>> coords) -> unsigned int {
                 if (coords.has_value())
                     return self.addPoint(coords.value().data());
                 return self.addPoint(nullptr);
             },
             py::arg("coordinates") = py::none(),
             "Add a new point. Pass a coordinate list or None for the origin.")
        .def("commit_element", &CHE::commitElement,
             "Mark the next element slot as valid and return its index.")
        .def("set_opposite", &CHE::setOpposite,
             py::arg("half_edge"), py::arg("opposite_half_edge"),
             "Set the opposite relationship for a half-edge.")
        .def("set_element_vertex", &CHE::setElementVertex,
             py::arg("half_edge"), py::arg("vertex"),
             "Change the vertex attached to a half-edge (element connectivity).")
        .def("reserve_space_for_elements", &CHE::reserveSpaceForElements,
             py::arg("number_elements"),
             "Pre-allocate space for additional elements.")
        .def("reserve_space_for_nodes", &CHE::reserveSpaceForNodes,
             py::arg("number_nodes"),
             "Pre-allocate space for additional nodes.")

        // --- utility ---
        .def("print", &CHE::print,
             "Print the data structure state to stdout.");

    // ──────────────────────────────────────────────
    // IBHM
    // ──────────────────────────────────────────────
    py::class_<IBHM, std::shared_ptr<IBHM>> pyIBHM(m, "IBHM",
        "Index-Based Half-edge for Hybrid Meshes data structure.");

    pyIBHM.attr("BORDER")    = static_cast<unsigned int>(IBHM::BORDER);
    pyIBHM.attr("COLLAPSED") = static_cast<unsigned int>(IBHM::COLLAPSED);

    pyIBHM
        .def(py::init<const std::vector<float> &, const std::vector<unsigned int> &,
                       const std::vector<unsigned int> &, unsigned int>(),
             py::arg("coordinates"), py::arg("elements_list"),
             py::arg("offset"), py::arg("number_coordinates"),
             "Construct an IBHM from coordinate, element, and offset vectors.")

        // --- query methods ---
        .def("he_element", &IBHM::heElement, py::arg("half_edge"),
             "Return the element index that contains the given half-edge.")
        .def("he_next", &IBHM::heNext, py::arg("half_edge"),
             "Return the next half-edge in the element orientation.")
        .def("he_previous", &IBHM::hePrevious, py::arg("half_edge"),
             "Return the previous half-edge in the element orientation.")
        .def("he_vertex_index", &IBHM::heVertexIndex, py::arg("half_edge"),
             "Return the vertex index attached to the given half-edge.")
        .def("he_opposite", &IBHM::heOpposite, py::arg("half_edge"),
             "Return the opposite half-edge, or IBHM.BORDER / IBHM.COLLAPSED.")
        .def("is_border", &IBHM::isBorder, py::arg("half_edge"),
             "Check whether the vertex at the given half-edge lies on the mesh border.")
        .def("next_available_he", &IBHM::nextAvailableHE,
             "Return the next available half-edge index for new elements.")
        .def("first_element_he", &IBHM::firstElementHE, py::arg("element_index"),
             "Return the first half-edge index of the given element.")

        // --- size queries ---
        .def("number_points", &IBHM::numberPoints,
             "Return the number of valid points in the mesh.")
        .def("number_of_elements", &IBHM::numberOfElements,
             "Return the number of valid elements in the mesh.")
        .def("number_reserved_elements", &IBHM::numberReservedElements,
             "Return the number of reserved element slots.")
        .def("number_coordinates_by_vertex", &IBHM::numberCoordinatesByVertex,
             "Return the number of coordinate components per vertex.")

        // --- accessors ---
        .def("elements_list", &IBHM::elementsList, py::return_value_policy::reference_internal,
             "Return a read-only reference to the elements list.")
        .def("points", &IBHM::points, py::return_value_policy::reference_internal,
             "Return a read-only reference to the points (coordinates) vector.")

        // --- coordinate helpers ---
        .def("coordinates_from_half_edge",
             [](const IBHM &self, unsigned int halfEdge) {
                 const unsigned int dim = self.numberCoordinatesByVertex();
                 // IBHM stores float internally but coordinatesFromHalfEge writes double.
                 std::vector<double> coords(dim);
                 self.coordinatesFromHalfEge(halfEdge, coords.data());
                 return coords;
             },
             py::arg("half_edge"),
             "Return the coordinates of the vertex attached to the given half-edge.")
        .def("coordinates_from_index",
             [](const IBHM &self, unsigned int index) {
                 const unsigned int dim = self.numberCoordinatesByVertex();
                 std::vector<double> coords(dim);
                 self.coordinatesFromIndex(index, coords.data());
                 return coords;
             },
             py::arg("index"),
             "Return the coordinates of the vertex at the given index.")

        // --- mutation methods ---
        .def("add_point",
             [](IBHM &self, std::optional<std::vector<float>> coords) -> unsigned int {
                 if (coords.has_value())
                     return self.addPoint(coords.value().data());
                 return self.addPoint(nullptr);
             },
             py::arg("coordinates") = py::none(),
             "Add a new point. Pass a coordinate list or None for the origin.")
        .def("commit_element", &IBHM::commitElement, py::arg("element_size"),
             "Mark the next element slot as valid and return its index.")
        .def("set_opposite", &IBHM::setOpposite,
             py::arg("half_edge"), py::arg("opposite_half_edge"),
             "Set the opposite relationship for a half-edge.")
        .def("set_element_vertex", &IBHM::setElementVertex,
             py::arg("half_edge"), py::arg("vertex"),
             "Change the vertex attached to a half-edge (element connectivity).")
        .def("reserve_space_for_elements", &IBHM::reserveSpaceForElements,
             py::arg("number_positions"), py::arg("number_elements"),
             "Pre-allocate space for additional elements.")
        .def("reserve_space_for_nodes", &IBHM::reserveSpaceForNodes,
             py::arg("number_nodes"),
             "Pre-allocate space for additional nodes.")

        // --- utility ---
        .def("print", &IBHM::print,
             "Print the data structure state to stdout.");

    // ──────────────────────────────────────────────
    // CHEInterfaceElementBuilder
    // ──────────────────────────────────────────────
    py::class_<CHEInterfaceElementBuilder> pyCHEBuilder(m, "CHEInterfaceElementBuilder",
        "Inserts interface elements into a CHE mesh by splitting edges.");

    // Expose the OperatorType enum (read-only, for informational / debugging use).
    py::enum_<CHEInterfaceElementBuilder::OperatorType>(pyCHEBuilder, "OperatorType",
        "Types of topological operators used during interface element insertion.")
        .value("CANONICAL",     CHEInterfaceElementBuilder::OperatorType::CANONICAL)
        .value("SPLIT_ELEMENT", CHEInterfaceElementBuilder::OperatorType::SPLIT_ELEMENT)
        .value("EXPAND_EDGE",   CHEInterfaceElementBuilder::OperatorType::EXPAND_EDGE)
        .value("OPEN_HOLE",     CHEInterfaceElementBuilder::OperatorType::OPEN_HOLE)
        .value("UNDEFINED",     CHEInterfaceElementBuilder::OperatorType::UNDEFINED)
        .export_values();

    pyCHEBuilder
        // The constructor takes a raw pointer. We accept a shared_ptr and keep it alive
        // via py::keep_alive so the CHE outlives the builder.
        .def(py::init([](std::shared_ptr<CHE> che) {
                 return std::make_unique<CHEInterfaceElementBuilder>(che.get());
             }),
             py::arg("che"),
             py::keep_alive<1, 2>(),  // builder (1) keeps che (2) alive
             "Create a builder that will modify the given CHE mesh.")
        .def("insert_interface_elements", &CHEInterfaceElementBuilder::insertInterfaceElements,
             py::arg("edges"),
             "Insert interface elements along the specified edges.\n\n"
             "Each edge is identified by one of its two half-edges.")
        .def("compute_number_of_new_vertices", &CHEInterfaceElementBuilder::computeNumberOfNewVertices,
             py::arg("edges"),
             "Compute how many new vertices would be created for the given edges.");

    // ──────────────────────────────────────────────
    // IBHMInterfaceElementBuilder
    // ──────────────────────────────────────────────
    py::class_<IBHMInterfaceElementBuilder> pyIBHMBuilder(m, "IBHMInterfaceElementBuilder",
        "Inserts interface elements into an IBHM hybrid mesh by splitting edges.");

    py::enum_<IBHMInterfaceElementBuilder::OperatorType>(pyIBHMBuilder, "OperatorType",
        "Types of topological operators used during interface element insertion.")
        .value("CANONICAL",     IBHMInterfaceElementBuilder::OperatorType::CANONICAL)
        .value("SPLIT_ELEMENT", IBHMInterfaceElementBuilder::OperatorType::SPLIT_ELEMENT)
        .value("EXPAND_EDGE",   IBHMInterfaceElementBuilder::OperatorType::EXPAND_EDGE)
        .value("OPEN_HOLE",     IBHMInterfaceElementBuilder::OperatorType::OPEN_HOLE)
        .value("UNDEFINED",     IBHMInterfaceElementBuilder::OperatorType::UNDEFINED)
        .export_values();

    pyIBHMBuilder
        .def(py::init([](std::shared_ptr<IBHM> ibhm) {
                 return std::make_unique<IBHMInterfaceElementBuilder>(ibhm.get());
             }),
             py::arg("ibhm"),
             py::keep_alive<1, 2>(),  // builder (1) keeps ibhm (2) alive
             "Create a builder that will modify the given IBHM mesh.")
        .def("insert_interface_elements", &IBHMInterfaceElementBuilder::insertInterfaceElements,
             py::arg("edges"),
             "Insert interface elements along the specified edges.\n\n"
             "Each edge is identified by one of its two half-edges.")
        .def("compute_number_of_new_vertices", &IBHMInterfaceElementBuilder::computeNumberOfNewVertices,
             py::arg("edges"),
             "Compute how many new vertices would be created for the given edges.");
}