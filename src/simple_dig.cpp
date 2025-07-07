#include <ios>
#include <iostream>
#include <fstream>
#include "turtle.hpp"

int main()
{
    char descent;
    int width, length, height;
    std::cout << "Width: ";
    std::cin >> width;
    std::cout << "\nLength: ";
    std::cin >> length;
    std::cout << "\nHeight: ";
    std::cin >> height;
    std::cout << "\nDescent? (Y/N) ";
    std::cin >> descent;
    descent = descent == 'y' || descent == 'Y';

    std::fstream output = std::fstream("dig-output.bin", std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);

    TurtleState state(width * length * height * 2 + 2048);

    if (descent)
    {
        MoveByGlobal(state, 0, -1, 0, true, true);

        for (int cz = 0; cz < height; cz++)
        {
            WriteAction(state, TurtleAction::DigDown);
            MoveByGlobal(state, 0, 0, -1, true, true);
        }
    }

    ConstructParallelepiped(state, 0, 0, descent ? -height : 0, width, length, descent ? 0 : height, true);
    MoveToGlobal(state, 0, 0, 0, false, false);

    output.write("\0\0", 2); //skip material data
    output.write(reinterpret_cast<char*>(state.Instructions), state.InstructionsPos);
    output.close();

    return 0;
}
