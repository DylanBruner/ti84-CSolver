#include <ti/vars.h>
#include <stdint.h>
#include <ti/screen.h>
#include <ti/getcsc.h>
#include <ti/getkey.h>
#include <ti/real>
#include <ti/tokens.h>
#include <cstring>
#include <sys/timers.h>
#include <time.h>
#include <cmath>

using namespace ti::literals;

uint64_t timeInTryEquations = 0;
uint64_t timeInIncrementVariables = 0;
uint64_t evalTime = 0;
uint64_t checkTime = 0;
uint64_t variableSettingTime = 0;

static char *floatToString(float value) {
    static char str[20];
    real_t val = os_FloatToReal(value);
    os_RealToStr(str, &val, 0, 0, -1);
    return str;
}

static char* intToYVar(uint8_t i) {
    if (i == 0) return OS_VAR_Y0;
    if (i == 1) return OS_VAR_Y1;
    if (i == 2) return OS_VAR_Y2;
    if (i == 3) return OS_VAR_Y3;
    if (i == 4) return OS_VAR_Y4;
    if (i == 5) return OS_VAR_Y5;
    if (i == 6) return OS_VAR_Y6;
    if (i == 7) return OS_VAR_Y7;
    if (i == 8) return OS_VAR_Y8;
    if (i == 9) return OS_VAR_Y9;
    return nullptr;
}

static void getEquations(equ_t* equations[10]) {
    equations[0] = os_GetEquationData(OS_VAR_Y0, 0);
    equations[1] = os_GetEquationData(OS_VAR_Y1, 0);
    equations[2] = os_GetEquationData(OS_VAR_Y2, 0);
    equations[3] = os_GetEquationData(OS_VAR_Y3, 0);
    equations[4] = os_GetEquationData(OS_VAR_Y4, 0);
    equations[5] = os_GetEquationData(OS_VAR_Y5, 0);
    equations[6] = os_GetEquationData(OS_VAR_Y6, 0);
    equations[7] = os_GetEquationData(OS_VAR_Y7, 0);
    equations[8] = os_GetEquationData(OS_VAR_Y8, 0);
    equations[9] = os_GetEquationData(OS_VAR_Y9, 0);
}

static bool contains(char* data, char c) {
    for (uint8_t i = 0; i < strlen(data); i++) {
        if (data[i] == c)
            return true;
    }
    return false;
}

static char* extractVariables(equ_t* equation) {
    char *variables = (char *)malloc(50);
    for (uint8_t i = 0; i < equation->len; i++) {
        if (contains(variables, equation->data[i]))
            continue;

        if (equation->data[i] >= OS_TOK_A && equation->data[i] <= OS_TOK_Z) { // A -> Z
            variables[strlen(variables)] = equation->data[i];
        }
        //TODO: Add support for other variables
    }

    return variables;
}

static char* extractVariables(equ_t* equations[10]) {
    char *variables = (char *)malloc(50);
    for (uint8_t i = 0; i < 10; i++) {
        if (equations[i]->len == 0)
            continue;

        char *vars = extractVariables(equations[i]);
        for (uint8_t j = 0; j < strlen(vars); j++) {
            if (contains(variables, vars[j]))
                continue;

            variables[strlen(variables)] = vars[j];
        }
    }

    return variables;
}

real_t REAL_ONE = os_FloatToReal(1.0f);

static bool tryEquations(equ_t* equations[10]) {
    for (uint8_t i = 0; i < 10; i++) {
        if (equations[i]->len == 0)
            continue;

        clock_t start = clock();
        os_EvalVar(intToYVar(i));
        evalTime += clock() - start;


        start = clock();
        uint8_t type;
        void *ans;

        ans = os_GetAnsData(&type);
        if (ans != NULL && type == OS_TYPE_REAL && os_RealCompare((real_t *)ans, &REAL_ONE) != 0)
            return false;
        checkTime += clock() - start;
    }

    return true;
}

static char* getVarRef(char var) {
    static char varRefCache[26][3] = {0};

    int varIndex = var - 'A';

    if (varRefCache[varIndex][0] != 0)
        return varRefCache[varIndex];

    varRefCache[varIndex][0] = '\x41' + varIndex;
    varRefCache[varIndex][1] = '\0';
    varRefCache[varIndex][2] = '\0';

    return varRefCache[varIndex];
}

bool increment_variables(float* values, float min, float max, float step_size, size_t num_vars) {
    const float epsilon = 1e-6;

    for (size_t i = 0; i < num_vars; i++) {
        values[i] = round((values[i] + step_size) / step_size) * step_size;
        
        if (values[i] <= max + epsilon)
            return false;
            
        values[i] = min;
    }
    return true;
}


/*
 * Loop through every possible combination of variables and solve the equations
*/
static bool solve(equ_t* equations[10], char* variables, float min=-10, float max=10, float step=1) {
    size_t num_vars = strlen(variables);
    float values[num_vars];

    // Initialize values array
    for (size_t i = 0; i < num_vars; i++) {
        values[i] = min;
    }

    while (true) {
        if (os_GetCSC() == sk_Clear)
            return false;

        clock_t start = clock();
        for (size_t i = 0; i < num_vars; i++) {
            real_t val = os_FloatToReal(values[i]);
            char* varRef = getVarRef(variables[i]);

            os_SetRealVar(varRef, &val);
        }
        variableSettingTime += clock() - start;

        // Try the equations with the current set of variables
        start = clock();
        if (tryEquations(equations))
            return true;
        timeInTryEquations += clock() - start;

        // Increment variables for the next combination
        start = clock();
        if (increment_variables(values, min, max, step, num_vars))
            break;
        timeInIncrementVariables += clock() - start;
    }

    return false;
}

int main(void) {
    // get the min, max and step size from the user
    char* minBuffer = (char *)malloc(10);
    char* maxBuffer = (char *)malloc(10);
    char* stepBuffer = (char *)malloc(10);

    os_GetStringInput("Min: ", minBuffer, 10);
    os_NewLine();
    os_GetStringInput("Max: ", maxBuffer, 10);
    os_NewLine();
    os_GetStringInput("Step: ", stepBuffer, 10);

    float min = atof(minBuffer);
    float max = atof(maxBuffer);
    float step = atof(stepBuffer);

    free(minBuffer);
    free(maxBuffer);
    free(stepBuffer);

    os_PutStrFull(floatToString(min));
    os_PutStrFull(", ");
    os_PutStrFull(floatToString(max));
    os_PutStrFull(", ");
    os_PutStrFull(floatToString(step));
    os_NewLine();

    delay(1000);

    os_ClrHome();

    equ_t* equations[10];
    char* variables;
    getEquations(equations);
    variables = extractVariables(equations);

    uint8_t equation_count = 0;
    for (uint8_t i = 0; i < 10; i++) {
        if (equations[i]->len > 0)
            equation_count++;
    }

    os_PutStrFull("Eqs: ");
    os_PutStrFull(floatToString(equation_count));
    os_PutStrFull(", Vars: ");
    os_PutStrFull(variables);
    os_NewLine();

    os_PutStrFull("Solving...");
    os_NewLine();

    clock_t start = clock();

    solve(equations, variables, min, max, step);

    if (tryEquations(equations)) {
        os_PutStrFull("Success!");
    } else {
        os_PutStrFull("Failed!");
    }

    clock_t end = clock();
    double time = (double)(end - start) / CLOCKS_PER_SEC;

    os_NewLine();
    os_NewLine();
    os_PutStrFull("Total Time: ");
    os_PutStrFull(floatToString(time));
    os_NewLine();

    os_PutStrFull("TryEquations: ");
    os_PutStrFull(floatToString((double)timeInTryEquations / CLOCKS_PER_SEC));
    os_NewLine();

    os_PutStrFull("Eval: ");
    os_PutStrFull(floatToString((double)evalTime / CLOCKS_PER_SEC));
    os_NewLine();

    os_PutStrFull("Check: ");
    os_PutStrFull(floatToString((double)checkTime / CLOCKS_PER_SEC));
    os_NewLine();

    os_PutStrFull("Var Setting: ");
    os_PutStrFull(floatToString((double)variableSettingTime / CLOCKS_PER_SEC));
    os_NewLine();

    os_PutStrFull("Increment: ");
    os_PutStrFull(floatToString((double)timeInIncrementVariables / CLOCKS_PER_SEC));
    os_NewLine();

    free(variables);
    while (!os_GetCSC());
}