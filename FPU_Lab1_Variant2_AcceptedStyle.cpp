// Лабораторная работа №1. Математический сопроцессор FPU (x87)
// Вариант 2. C++ + ассемблерная вставка __asm.
//
// F(x) = ((tg(cuberoot((x-1)^2)) + exp(3-3*x)) /
//         (2*lg(10+abs(1-x*x)))) * (3-x)
// x от 1 до 3 включительно; 10 равномерных точек.
//
// Для Microsoft Visual Studio: конфигурация x86 / Win32 (не x64).

#include <iostream>
#include <iomanip>
#include <cmath>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  // Исходный файл сохраняется в UTF-8
#endif

    float x1 = 1.0f, x2 = 3.0f;
    float change = 0.0f, x[10] = {}, y[10] = {};
    int nine = 9, index = 0;
    const double twoThirds = 2.0 / 3.0;
    const double three = 3.0;
    const double ten = 10.0;
    double tanPart = 0.0, numerator = 0.0, denominator = 0.0;

    __asm {
        finit                           // Инициализация сопроцессора

        // change = (x2-x1)/9
        fld dword ptr [x2]
        fsub dword ptr [x1]
        fidiv dword ptr [nine]
        fstp dword ptr [change]

        mov ecx, 10                     // 10 точек
        xor esi, esi                    // Смещение по массивам x[] и y[]

    lp:
        // x[i] = x1 + i*change
        // Вычисление от x1, чтобы не накапливать ошибку шага.
        mov eax, esi
        shr eax, 2                      // i = esi / 4, float занимает 4 байта
        mov index, eax
        fild dword ptr [index]
        fmul dword ptr [change]
        fadd dword ptr [x1]
        fstp dword ptr x[esi]

        // t = кубический корень из (x-1)^2 = (x-1)^(2/3), x >= 1.
        fld dword ptr x[esi]
        fld1
        fsubp st(1), st(0)             // ST(0) = x-1
        ftst                            // При x=1 нельзя брать log2(0)
        fstsw ax
        sahf
        jz cube_zero

        fld qword ptr [twoThirds]
        fxch st(1)
        fyl2x                           // z = (2/3)*log2(x-1)

        // 2^z = 2^(n+f) = 2^f * 2^n, -0.5 <= f <= 0.5.
        fld st(0)                       // z, z
        frndint                         // n, z
        fsub st(1), st(0)              // n, z-n
        fxch st(1)                     // f, n
        f2xm1                           // 2^f - 1, n
        fld1
        faddp st(1), st(0)             // 2^f, n
        fscale                          // 2^z, n
        fstp st(1)                     // Убираем n, оставляем 2^z
        jmp cube_ready

    cube_zero:
        fstp st(0)                     // Убираем x-1 = 0
        fldz                            // t = 0

    cube_ready:
        fptan                           // ST(0)=1, ST(1)=tg(t)
        fstp st(0)                     // Убираем единицу
        fstp qword ptr [tanPart]        // Сохраняем tg(t)

        // e^(3-3*x) = 2^((3-3*x)*log2(e))
        fld dword ptr x[esi]
        fmul qword ptr [three]
        fchs
        fadd qword ptr [three]
        fldl2e
        fmulp st(1), st(0)             // z=(3-3*x)*log2(e)

        fld st(0)
        frndint
        fsub st(1), st(0)
        fxch st(1)
        f2xm1
        fld1
        faddp st(1), st(0)
        fscale
        fstp st(1)
        fadd qword ptr [tanPart]
        fstp qword ptr [numerator]      // числитель = tg(t)+exp(3-3*x)

        // знаменатель = 2*lg(10+abs(1-x*x))
        fld dword ptr x[esi]
        fmul dword ptr x[esi]           // x^2
        fld1
        fsubp st(1), st(0)             // x^2 - 1
        fabs                            // |x^2-1| = |1-x^2|
        fadd qword ptr [ten]
        fldlg2                          // log10(2), аргумент логарифма
        fxch st(1)                     // аргумент, log10(2)
        fyl2x                           // lg(аргумент)
        fadd st(0), st(0)              // 2*lg(...)
        fstp qword ptr [denominator]

        // y[i] = numerator/denominator*(3-x)
        fld qword ptr [numerator]
        fdiv qword ptr [denominator]
        fld qword ptr [three]
        fsub dword ptr x[esi]
        fmulp st(1), st(0)
        fstp dword ptr y[esi]

        add esi, 4                     // Следующие элементы массивов (float = 4 байта)
        dec ecx
        jnz lp                         // Пока не обработаны все 10 точек
    }

    cout << fixed << setprecision(4);
    cout << "FPU, variant 2 (x87)\n";
    for (int i = 0; i < 10; ++i) {
        // Проверка ОДЗ для полученной точки. Для всех 10 точек [1;3] она выполняется.
        const double xi = x[i];
        const double t = pow(xi - 1.0, 2.0 / 3.0);
        const double logArg = 10.0 + fabs(1.0 - xi * xi);
        if (logArg <= 0.0 || 2.0 * log10(logArg) == 0.0 ||
            fabs(cos(t)) < 1e-12 || !isfinite(y[i])) {
            cout << "x=" << setw(6) << x[i]
                 << ": Значение аргумента не входит в ОДЗ\n";
        } else {
            float yi = y[i];
            if (fabs(yi) < 0.00005f) yi = 0.0f; // 0 вместо -0.0000
            cout << "x=" << setw(6) << x[i] << "  y=" << setw(9) << yi << '\n';
        }
    }
    return 0;
}
