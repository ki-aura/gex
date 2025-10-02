#include <stdio.h>
#include <newt.h>

int main(void) {
    newtInit();
    newtCls();

    // --- Popup message ---
    newtWinMessage("Popup Example", "OK", "This is a popup message!");

    // --- Menu ---
    char *menu_items[] = { "Option 1", "Option 2", "Option 3", NULL };
    int selected_index = 0;
    newtWinMenu("Menu Example", "Choose an option:", 40, 0, 0, 3,
                menu_items, &selected_index, "OK", NULL);
    printf("Selected menu index: %d\n", selected_index);

    // --- Form with Text Entry and Checkbox ---
    newtComponent form = newtForm(NULL, NULL, 0);

    const char *entry_result = NULL;
    newtComponent entry = newtEntry(2, 1, "default text", 20, &entry_result, 0);
    newtFormAddComponent(form, entry);

    char chk_result;
    newtComponent checkbox = newtCheckbox(2, 3, "Check me", ' ', NULL, &chk_result);
    newtFormAddComponent(form, checkbox);

    // Submit button to exit form
    newtComponent ok_button = newtButton(2, 5, "OK");
    newtFormAddComponent(form, ok_button);

    struct newtExitStruct es;
    newtFormRun(form, &es);

    printf("Entered text: %s\n", entry_result);
    printf("Checkbox value: %c\n", chk_result);

    // Clean up
    newtFormDestroy(form);
    newtFinished();

    return 0;
}
