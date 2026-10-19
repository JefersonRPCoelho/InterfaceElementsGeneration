#pragma once
#include <unordered_map>
#include "IBHMInterfaceElementBuilder.h"


/**
 * @brief Read-only access to the private state of IBHMInterfaceElementBuilder, for the tests and the test driver.
 *
 * The builder declares this class as a friend, so the tests can check the operator chosen for a vertex and the
 * builder's bookkeeping without changing its public interface. Only static functions; the library does not use it.
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
