/**
 * @file NeighborHood.h
 * @author qiming
 * @date 25-4-13
 * @brief Defines neighborhood move operations for FJSP (Flexible Job Shop Problem) optimization.
 */

#ifndef FJSP_NEIGHBORHOOD_MOVE_H
#define FJSP_NEIGHBORHOOD_MOVE_H
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

 /**
  * @enum Method
  * @brief Enumerates different types of neighborhood moves for FJSP optimization.
  *
  * These methods define different ways to modify a solution in the search space:
  * - BACK: Move an operation to a same machine's later position
  * - FRONT: Move an operation to a same machine's earlier position
  * - CHANGE_MACHINE_BACK: Change the machine assignment and move to a later position
  * - CHANGE_MACHINE_FRONT: Change the machine assignment and move to an earlier position
  */
enum class Method
{
    BACK,
    FRONT,
    CHANGE_MACHINE_BACK,
    CHANGE_MACHINE_FRONT
};

/**
 * @struct NeighborhoodMove
 * @brief Represents a specific move operation in the neighborhood search.
 *
 * This structure encapsulates the details of a move operation, including
 * the method type and the positions involved in the move.
 */
struct NeighborhoodMove
{
    Method method; ///< The type of move to perform
    int which; ///< The op_id of the operation to move
    int where; ///< The destination op_id for the operation

    /**
     * @brief Construct a new Neighborhood Move with specified parameters
     *
     * @param method The method type for this move
     * @param which The op_id of the operation to move
     * @param where The destination op_id for the operation
     */
    constexpr NeighborhoodMove(Method method, int which, int where) noexcept :
        method(method), which(which), where(where)
    {
    }

    /**
     * @brief Default constructor that initializes to BACK method with zero op_id
     */
    constexpr NeighborhoodMove() noexcept : method(Method::BACK), which(0), where(0) {}

    /**
     * @brief Stream insertion operator for NeighborhoodMove
     *
     * @param os The output stream
     * @param move The move to output
     * @return std::ostream& The modified output stream
     */
    friend std::ostream& operator<<(std::ostream& os, const NeighborhoodMove& move)
    {
        os << "method: " << move.str() << " which: " << move.which << " where: " << move.where;
        return os;
    }

    /**
     * @brief Convert the method enum to a string representation
     *
     * @return std::string The string representation of the method
     * @throws std::invalid_argument If the method has an unknown value
     */
    [[nodiscard]] std::string str() const
    {
        static constexpr std::array<std::string_view, 4> methodToString = { "BACK", "FRONT", "CHANGE_MACHINE_BACK",
                                                                           "CHANGE_MACHINE_FRONT" };

        const auto index = static_cast<size_t>(method);
        if (index >= methodToString.size())
        {
            throw std::invalid_argument("Unknown method enum value");
        }
        return std::string(methodToString[index]);
    }
};

#endif // FJSP_NEIGHBORHOOD_MOVE_H
