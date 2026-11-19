
#include <iostream>
#include <iomanip>
#include <windows.h>
using namespace std;

int main() {
    SetConsoleOutputCP(65001);

    float x1 = 1, x2 = 2.968701, change, x, y;
    float eps = 0.000001f;
    int tmp, valid;

    __asm {
        finit

        // 1. Шаг (x2-x1)/9
        fld x2
        fsub x1
        mov tmp, 9
        fild tmp
        fdivp st(1), st(0)
        fstp change
    }

    cout << fixed << setprecision(4);

    for (int i = 0; i < 10; i++) {
        valid = 1;

        __asm {
            finit

            // 2. x = x1 + i*change
            cmp i, 9
            je last_x
            fild i
            fmul change
            fadd x1
            jmp x_ready

        last_x:
            fld x2

        x_ready:
            fstp x

            // 3. (x-1)^2
            fld x
            fld1
            fsubp st(1), st(0)
            fmul st(0), st(0)

            // 4. Кубический корень
            ftst
            fstsw ax
            sahf
            jz root_ready

            fld1
            fxch st(1)
            fyl2x
            mov tmp, 3
            fidiv tmp

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
            // 5. Проверка ОДЗ
            fld st(0)
            fcos
            fabs
            fcomp eps
            fstsw ax
            sahf
            jbe not_domain

            // 6. Тангенс
            fptan
            fstp st(0)

            // 7. Экспонента e^(3-3x)
            fld x
            mov tmp, 3
            fimul tmp
            fisubr tmp
            fldl2e
            fmulp st(1), st(0)

            fld st(0)
            frndint
            fsub st(1), st(0)
            fxch st(1)
            f2xm1
            fld1
            faddp st(1), st(0)
            fscale
            fstp st(1)

            // 8. Числитель
            faddp st(1), st(0)

            // 9. Знаменатель
            fld x
            fmul st(0), st(0)
            fld1
            fsubp st(1), st(0)
            fabs
            mov tmp, 10
            fiadd tmp
            fldlg2
            fxch st(1)
            fyl2x
            fadd st(0), st(0)

            // 10. Итоговая функция
            fdivp st(1), st(0)
            mov tmp, 3
            fild tmp
            fsub x
            fmulp st(1), st(0)

            // Удаляем знак отрицательного нуля
            ftst
            fstsw ax
            sahf
            jnz save_y
            fabs

        save_y:
            fstp y
            jmp calculation_done

        not_domain:
            fstp st(0)
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
