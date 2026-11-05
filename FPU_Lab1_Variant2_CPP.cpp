// Лабораторная работа 1. Математический сопроцессор FPU (x87)
// Вариант 2. Microsoft Visual C++ (Win32/x86), встроенный ассемблер __asm.
// Важно: MSVC x64 и MSVC ARM64 не поддерживают __asm.
//
// F(x) = (tan(cuberoot((x-1)^2)) + exp(3-3*x)) * (3-x)
//        -------------------------------------------------
//                 2 * lg(10 + abs(1-x*x))
//
// Десять точек включительно на интервале [1;3].

#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>

int main() {
    const double x1 = 1.0;
    const double x2 = 3.0;
    const int count = 10;
    const double step = (x2 - x1) / (count - 1);

    const double twoThirds = 2.0 / 3.0;
    const double three = 3.0;
    const double ten = 10.0;

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "FPU, variant 2\n";
    std::cout << "x        F(x)\n";

    for (int i = 0; i < count; ++i) {
        const double x = x1 + step * i;
        double tanPart = 0.0;
        double numerator = 0.0;
        double denominator = 0.0;
        double result = 0.0;

        // На [1;3] аргумент логарифма >=10, знаменатель >0.
        // Значение (x-1)^(2/3) пересекает pi/2 около 2.9687,
        // но ни одна из десяти расчетных точек не совпадает с полюсом tan.
        const double t = std::pow(x - 1.0, 2.0 / 3.0);
        const double logArg = 10.0 + std::fabs(1.0 - x * x);
        const double pi = std::acos(-1.0);
        if (logArg <= 0.0 || std::fabs(std::cos(t)) < 1e-12 ||
            2.0 * std::log10(logArg) == 0.0) {
            std::cout << std::setw(8) << x
                      << "  Argument is outside the domain\n";
            continue;
        }

        // Основные расчеты выполняет FPU x87, а не функции C++.
        __asm {
            finit

            // t = (x - 1)^(2/3) = 2^((2/3)*log2(x-1)).
            fld qword ptr [x]          // ST(0)=x
            fld1                       // ST(0)=1, ST(1)=x
            fsubp st(1), st(0)         // ST(0)=x-1
            ftst                       // Отдельно обрабатываем x=1
            fstsw ax
            sahf
            jz cube_zero

            fld qword ptr [twoThirds]  // ST(0)=2/3, ST(1)=x-1
            fxch st(1)                 // ST(0)=x-1, ST(1)=2/3
            fyl2x                      // ST(0)=(2/3)*log2(x-1)

            // Возводим 2 в произвольную степень z:
            // z = n + f; 2^z = 2^n * 2^f, |f| <= 0.5.
            fld st(0)                  // z, z
            frndint                    // n, z
            fsub st(1), st(0)          // n, z-n
            fxch st(1)                 // f, n
            f2xm1                      // 2^f - 1, n
            fld1                       // 1, 2^f-1, n
            faddp st(1), st(0)         // 2^f, n
            fscale                     // 2^z, n
            fstp st(1)                 // 2^z (удаляем n)
            jmp cube_ready

        cube_zero:
            fstp st(0)                 // Удаляем 0 = x-1
            fldz                       // (x-1)^(2/3) = 0

        cube_ready:
            fptan                      // ST(0)=1, ST(1)=tan(t)
            fstp st(0)                 // ST(0)=tan(t)
            fstp qword ptr [tanPart]   // Сохраняем tan(t), стек пуст

            // e^(3-3*x) = 2^((3-3*x)*log2(e))
            fld qword ptr [x]
            fmul qword ptr [three]     // 3*x
            fchs                       // -3*x
            fadd qword ptr [three]     // 3-3*x
            fldl2e                     // log2(e), 3-3*x
            fmulp st(1), st(0)         // z = (3-3*x)*log2(e)

            fld st(0)                  // z, z
            frndint                    // n, z
            fsub st(1), st(0)          // n, z-n
            fxch st(1)                 // f, n
            f2xm1                      // 2^f-1, n
            fld1
            faddp st(1), st(0)         // 2^f, n
            fscale                     // 2^z, n
            fstp st(1)                 // 2^z
            fadd qword ptr [tanPart]   // tan(t)+e^(3-3*x)
            fstp qword ptr [numerator] // Числитель, стек пуст

            // 2*lg(10 + |1-x^2|)
            fld qword ptr [x]
            fmul qword ptr [x]         // x^2
            fld1
            fsubp st(1), st(0)         // x^2-1; модуль совпадает с |1-x^2|
            fabs
            fadd qword ptr [ten]       // 10+|1-x^2|
            fldlg2                     // log10(2), значение
            fxch st(1)                 // значение, log10(2)
            fyl2x                      // log10(2)*log2(значение)=lg(значение)
            fadd st(0), st(0)          // 2*lg(...)
            fstp qword ptr [denominator]

            // F(x) = (числитель / знаменатель)*(3-x)
            fld qword ptr [numerator]
            fdiv qword ptr [denominator]
            fld qword ptr [three]
            fsub qword ptr [x]         // 3-x, частное
            fmulp st(1), st(0)         // F(x)
            fstp qword ptr [result]    // Результат в C++ переменную
        }

        if (!std::isfinite(result)) {
            std::cout << std::setw(8) << x
                      << "  Argument is outside the domain\n";
        } else {
            if (std::fabs(result) < 0.00005) result = 0.0;
            std::cout << std::setw(8) << x << "  " << std::setw(9) << result << '\n';
        }
    }
    return 0;
}
