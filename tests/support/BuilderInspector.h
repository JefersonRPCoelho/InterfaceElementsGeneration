#pragma once
#include <cstddef>
#include <unordered_map>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"


/**
 * @brief Access to the private state of IBHMInterfaceElementBuilder, for the tests and the test driver.
 *
 * The builder declares this class as a friend, so the tests can check the operator chosen for a vertex and the
 * builder's bookkeeping without changing its public interface. The access is read-only, except for insertEdge(), a
 * test-only replica of the insertion used to test the equivalence of Split Element and Expand Edge. Only static
 * functions; the library does not use it.
 */
class BuilderInspector
{
public:
    using OperatorType = IBHMInterfaceElementBuilder::OperatorType;

    /**
     * @brief Get the operator the builder would apply now to the vertex of a half-edge.
     *
     * The half-edge must be an edge the builder accepts (an interior edge that is not part of an interface element).
     * @param builder The builder.
     * @param he The half-edge of the edge being opened that starts at the vertex.
     * @return The operator type.
     */
    [[nodiscard]] static OperatorType retrieveOperator(IBHMInterfaceElementBuilder &builder, const unsigned int he)
    {
        unsigned int availableVertexHE = 0, sharedElementHE = 0;
        return builder.retrieveOperator(he, availableVertexHE, sharedElementHE);
    }

    /**
     * @brief Check if an element is an interface element.
     * @param builder The builder.
     * @param element The element index.
     * @return True if the element was created by the builder.
     */
    [[nodiscard]] static bool isInterfaceElement(const IBHMInterfaceElementBuilder &builder,
                                                 const unsigned int element)
    {
        return builder._isInterfaceElement[element];
    }

    /**
     * @brief Check if a vertex belongs to an interface element.
     * @param builder The builder.
     * @param vertex The vertex index.
     * @return True if the vertex is part of an interface element.
     */
    [[nodiscard]] static bool isInInterfaceElement(const IBHMInterfaceElementBuilder &builder,
                                                   const unsigned int vertex)
    {
        return builder._inInterfaceElement[vertex];
    }

    /**
     * @brief Get the collapsed edges: vertex -> collapsed half-edge.
     * @param builder The builder.
     * @return The builder's map of collapsed edges.
     */
    [[nodiscard]] static const std::unordered_map<unsigned int, unsigned int> &collapsed(
        const IBHMInterfaceElementBuilder &builder)
    {
        return builder._collapsed;
    }

    /**
     * @brief Get the size of the interface element flags (one per element, plus the reserved ones).
     * @param builder The builder.
     * @return The size of _isInterfaceElement.
     */
    [[nodiscard]] static size_t numberOfElementFlags(const IBHMInterfaceElementBuilder &builder)
    {
        return builder._isInterfaceElement.size();
    }

    /**
     * @brief Get the size of the vertex flags (one per vertex).
     * @param builder The builder.
     * @return The size of _inInterfaceElement.
     */
    [[nodiscard]] static size_t numberOfVertexFlags(const IBHMInterfaceElementBuilder &builder)
    {
        return builder._inInterfaceElement.size();
    }

    /**
     * @brief Insert one interface element as insertInterfaceElements() does, optionally applying Expand Edge where the
     * builder would apply Split Element (test of the equivalence of the two operators).
     *
     * It is a test-only replica of the body of insertInterfaceElements() for one edge that the builder accepts (the
     * input is not validated). Without the replacement, it must give exactly the same mesh as
     * insertInterfaceElements(): the tests check it, so a divergence between the replica and the builder is detected.
     * @param builder The builder.
     * @param a A half-edge of the edge to open.
     * @param expandInsteadOfSplit Apply Expand Edge instead of Split Element.
     * @return The number of vertices (0, 1 or 2) where Split Element was replaced.
     */
    static unsigned int insertEdge(IBHMInterfaceElementBuilder &builder, const unsigned int a,
                                   const bool expandInsteadOfSplit)
    {
        IBHM &mesh = *builder._ibhm;
        const unsigned int b = mesh.heOpposite(a);

        // Reservations, as in insertInterfaceElements() for a single edge.
        const unsigned int numberOfNewVertices = builder.computeNumberOfNewVertices({a});
        mesh.reserveSpaceForElements(4, 1);
        const size_t numberOfElementFlags = static_cast<size_t>(mesh.numberOfElements()) + 1;
        if (numberOfElementFlags > builder._isInterfaceElement.size())
        {
            builder._isInterfaceElement.reserve(numberOfElementFlags);
            builder._isInterfaceElement.resize(numberOfElementFlags, false);
        }
        mesh.reserveSpaceForNodes(numberOfNewVertices);
        const size_t numberOfVertexFlags = static_cast<size_t>(mesh.numberPoints()) + numberOfNewVertices;
        if (numberOfVertexFlags > builder._inInterfaceElement.size())
        {
            builder._inInterfaceElement.reserve(numberOfVertexFlags);
            builder._inInterfaceElement.resize(numberOfVertexFlags, false);
        }

        // The new element, the operators at A (with h3) and at B (with h1), and the middle edges.
        const unsigned int h0 = mesh.nextAvailableHE();
        const unsigned int element = mesh.commitElement(4);
        const unsigned int replaced = applyOperator(builder, a, h0 + 3, expandInsteadOfSplit) +
                                      applyOperator(builder, b, h0 + 1, expandInsteadOfSplit);
        mesh.setOpposite(h0, b);
        mesh.setOpposite(h0 + 2, a);
        builder._isInterfaceElement[element] = true;
        return replaced;
    }

private:
    /**
     * @brief Choose and apply the operator at the vertex of a half-edge, as insertInterfaceElements() does.
     * @param builder The builder.
     * @param he The half-edge of the new edge that starts at the vertex.
     * @param elementHE The short side of the new element at the vertex.
     * @param expandInsteadOfSplit Apply Expand Edge instead of Split Element.
     * @return 1 if Split Element was replaced, 0 otherwise.
     */
    static unsigned int applyOperator(IBHMInterfaceElementBuilder &builder, const unsigned int he,
                                      const unsigned int elementHE, const bool expandInsteadOfSplit)
    {
        unsigned int availableVertexHE = 0, sharedElementHE = 0;
        OperatorType op = builder.retrieveOperator(he, availableVertexHE, sharedElementHE);
        const unsigned int replaced = op == OperatorType::SPLIT_ELEMENT && expandInsteadOfSplit ? 1 : 0;
        if (replaced == 1)
        {
            op = OperatorType::EXPAND_EDGE;
        }

        switch (op)
        {
            case OperatorType::CANONICAL:
                builder.canonical(he, elementHE);
                break;
            case OperatorType::SPLIT_ELEMENT:
                builder.splitElement(he, availableVertexHE, sharedElementHE, elementHE);
                break;
            case OperatorType::EXPAND_EDGE:
                builder.expandEdge(he, elementHE);
                break;
            case OperatorType::OPEN_HOLE:
            case OperatorType::UNDEFINED:
            default:
                builder.openHole(he, elementHE);
                break;
        }
        return replaced;
    }

public:
    /**
     * @brief Get the name of an operator type, as used in the report and in the driver's output.
     * @param op The operator type.
     * @return The name, for example "SPLIT_ELEMENT".
     */
    [[nodiscard]] static const char *operatorName(const OperatorType op)
    {
        switch (op)
        {
            case OperatorType::CANONICAL:
                return "CANONICAL";
            case OperatorType::SPLIT_ELEMENT:
                return "SPLIT_ELEMENT";
            case OperatorType::EXPAND_EDGE:
                return "EXPAND_EDGE";
            case OperatorType::OPEN_HOLE:
                return "OPEN_HOLE";
            case OperatorType::UNDEFINED:
            default:
                return "UNDEFINED";
        }
    }
};
