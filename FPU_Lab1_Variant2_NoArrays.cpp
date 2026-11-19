// Лабораторная работа 1. FPU (x87), вариант 2.
// Visual Studio: конфигурация x86 (Win32). Компилятор MSVC: /utf-8.
// Все математические вычисления выполняются в __asm.

#include <iostream>
#include <iomanip>
#include <windows.h>
using namespace std;

int main() {
    SetConsoleOutputCP(65001);

    float x1 = 1.0f, x2 = 3.0f, change, x, y;
    float eps = 0.000001f;
    int tmp, valid;

    __asm {
        finit

        // 1. Шаг между 10 точками: (x2-x1)/9
        fld dword ptr x2
        fsub dword ptr x1
        mov tmp, 9
        fild dword ptr tmp
        fdivp st(1), st(0)
        fstp dword ptr change
    }

    cout << fixed << setprecision(4);

    for (int i = 0; i < 10; i++) {
        valid = 1;

        __asm {
            finit

            // 2. Текущее x = x1 + i*change
            // Последняя точка берётся точно из x2.
            cmp i, 9
            je last_x
            fild dword ptr i
            fmul dword ptr change
            fadd dword ptr x1
            jmp x_ready
        last_x:
            fld dword ptr x2
        x_ready:
            fstp dword ptr x

            // 3. Квадрат (x-1)^2
            fld dword ptr x
            fld1
            fsubp st(1), st(0)
            fmul st(0), st(0)

            // 4. Кубический корень: 2^(log2((x-1)^2)/3)
            ftst
            fstsw ax
            sahf
            jz root_ready               // кубический корень из нуля = ноль

            fld1
            fxch st(1)
            fyl2x
            mov tmp, 3
            fidiv dword ptr tmp

            // 2^p: разделяем p на целую и дробную части
            fld st(0)
            frndint
            fsub st(1), st(0)
            fxch st(1)
            f2xm1
            fld1
            faddp st(1), st(0)
            fscale
            fstp st(1)

        root_ready:
            // 5. Проверка ОДЗ для тангенса: abs(cos(root)) > eps
            fld st(0)
            fcos
            fabs
            fcomp dword ptr eps
            fstsw ax
            sahf
            jbe not_domain

            // 6. Тангенс кубического корня
            fptan
            fstp st(0)                 // удаляем дополнительную единицу

            // 7. Экспонента e^(3-3x)
            fld dword ptr x
            mov tmp, 3
            fimul dword ptr tmp
            fisubr dword ptr tmp
            fldl2e
            fmulp st(1), st(0)

            // 2^p через F2XM1 и FSCALE
            fld st(0)
            frndint
            fsub st(1), st(0)
            fxch st(1)
            f2xm1
            fld1
            faddp st(1), st(0)
            fscale
            fstp st(1)

            // 8. Числитель: tan(root) + e^(3-3x)
            faddp st(1), st(0)

            // 9. Знаменатель: 2*lg(10+abs(1-x^2))
            fld dword ptr x
            fmul st(0), st(0)
            fld1
            fsubp st(1), st(0)
            fabs
            mov tmp, 10
            fiadd dword ptr tmp
            fldlg2
            fxch st(1)
            fyl2x
            fadd st(0), st(0)

            // 10. Итог: числитель / знаменатель * (3-x)
            fdivp st(1), st(0)
            mov tmp, 3
            fild dword ptr tmp
            fsub dword ptr x
            fmulp st(1), st(0)

            // Убираем знак у отрицательного нуля
            ftst
            fstsw ax
            sahf
            jnz save_y
            fabs
        save_y:
            fstp dword ptr y
            jmp calculation_done

        not_domain:
            fstp st(0)                 // очищаем стек FPU
            mov valid, 0

        calculation_done:
        }

        cout << "x=" << setw(6) << x << "  ";
        if (valid)
            cout << "y=" << y << '\n';
        else
            cout << "Значение аргумента не входит в ОДЗ\n";
    }

    return 0;
}
