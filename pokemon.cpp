void createPokemon(worldPokemon *p) {
    int pokeChoice = (rand() % 1092) + 1;

    // Set pokemon_id, species_id, & base_experience from pokemon.csv
    p->pokeId = pokemon[pokeChoice].id;
    p->speciesId = pokemon[pokeChoice].species_id;
    p->exp = pokemon[pokeChoice].base_experience;

    // Random values/chance
    p->gender = (rand() % 2 == 0);
    p->shiny = (rand() % 8192 == 4);

    p->hpIV = rand() % 16;
    p->attackIV = rand() % 16;
    p->defenseIV = rand() % 16;
    p->specialAttackIV = rand() % 16;
    p->specialDefenseIV = rand() % 16;
    p->speedIV = rand() % 16;

    // Setting name/identifier & level growth rate from pokemon_species.csv
    for (int i = 0; i < 899; i++) {
        if (p->speciesId == species[i].id) {
            strcpy(p->name, species[i].identifier);
            p->name[0] = toupper(p->name[0]);
            p->lvlRate = species[i].growth_rate_id;
        }
    }

    // Setting level from experience.csv using lvlRate
    int j = 0;
    for (int i = 0; i < 601; i++) {
        if (experience[i].growth_rate_id == p->lvlRate) {
            p->levels[j].growth_rate_id = experience[i].growth_rate_id;
            p->levels[j].level = experience[i].level;
            p->levels[j].experience = experience[i].experience;
            j++;
        }
    }
    j = 0;
    while (p->exp > p->levels[j].experience) {
        j++;
    }
    p->level = p->levels[j-1].level;

    // Get Base Stats from pokemon_stats.csv
    for (int i = 0; i < 6553; i++) {
        if(pokemon_stats[i].pokemon_id == p->pokeId) {
            switch(pokemon_stats[i].stat_id) {
                case 1 :
                    p->hp = pokemon_stats[i].base_stat;
                    break;
                case 2 :
                    p->attack = pokemon_stats[i].base_stat;
                    break;
                case 3 :
                    p->defense = pokemon_stats[i].base_stat;
                    break;
                case 4 :
                    p->specialAttack = pokemon_stats[i].base_stat;
                    break;
                case 5 :
                    p->specialDefense = pokemon_stats[i].base_stat;
                    break;
                case 6 :
                    p->speed = pokemon_stats[i].base_stat;
                    break;
            }
        }
    }

    // Find moves
    int learnables[250];
    j = 0;
    for (int i = 0; i < 528239; i++) {
        if (pokemon_moves[i].pokemon_id == p->speciesId
        && pokemon_moves[i].pokemon_move_method_id == 1) {
            learnables[j] = pokemon_moves[i].move_id;
            if (j + 1 < 250) {
                j++;
            }
        }
    }

    // pick potentially two moves
    int num1 = learnables[rand() % (j - 1)];
    int num2 = num1;
    while (num2 == num1) {
        num2 = learnables[rand() % (j - 1)];
    }

    j = 0;
    for (int i = 0; i < 845; i++) {
        if (moves[i].id == num1 || moves[i].id == num2) {
            p->pokeMoves[j].id = moves[i].id;
            strcpy(p->pokeMoves[j].identifier, moves[i].identifier);
            p->pokeMoves[j].generation_id = moves[i].generation_id;
            p->pokeMoves[j].type_id = moves[i].type_id;
            p->pokeMoves[j].power = moves[i].power;
            p->pokeMoves[j].pp = moves[i].pp;
            p->pokeMoves[j].accuracy = moves[i].accuracy;
            p->pokeMoves[j].priority = moves[i].priority;
            p->pokeMoves[j].target_id = moves[i].target_id;
            p->pokeMoves[j].damage_class_id = moves[i].damage_class_id;
            p->pokeMoves[j].effect_id = moves[i].effect_id;
            p->pokeMoves[j].effect_chance = moves[i].effect_chance;
            p->pokeMoves[j].contest_type_id = moves[i].contest_type_id;
            p->pokeMoves[j].contest_effect_id = moves[i].contest_effect_id;
            p->pokeMoves[j].super_contest_effect_id = moves[i].super_contest_effect_id;
            j++;
        }
    }
}

void levelPokemon(worldPokemon *p) {
    mvprintw(0, 0, "Attempting to level up %s!", p->name);

    // Update stats with provided formula
    p->hp = ((((p->hp + p->hpIV) * 2) * p->level) / 100) + p->level + 10;
}

void givePlayerPokemon() {
    // WINDOW *win = newwin(21, 80, 0, 0);
    // box(win, 0, 0);
    // // mvwprintw(win, 12, 2, "Press Any Key to exit");
    // wrefresh(win);
    // refresh();
    worldPokemon *option1 = static_cast<worldPokemon*>(malloc(sizeof(worldPokemon)));
    createPokemon(option1);
    option1->level = 1;
    option1->exp = 0;
    option1->shiny = 0;
    worldPokemon *option2 = static_cast<worldPokemon*>(malloc(sizeof(worldPokemon)));
    createPokemon(option2);
    option2->level = 1;
    option2->exp = 0;
    option2->shiny = 0;
    worldPokemon *option3 = static_cast<worldPokemon*>(malloc(sizeof(worldPokemon)));
    createPokemon(option3);
    option3->level = 1;
    option3->exp = 0;
    option3->shiny = 0;

    // int c = getch();
    WINDOW *win = newwin(19, 78, 2, 1);
    refresh();
    box(win, 0, 0);

    mvwprintw(win, 0, 10, "Before you begin your journey, you must pick your companion");
    
    mvwprintw(win, 2, 7, "%s", option1->name);
    mvwprintw(win, 3, 4, "Gender: %s", (option1->gender ? "Male" : "Female"));
    mvwprintw(win, 4, 4, "Level: %d", option1->level);
    mvwprintw(win, 6, 4, "HP: %d", option1->hp);
    mvwprintw(win, 7, 4, "ATK: %d", option1->attack);
    mvwprintw(win, 8, 4, "DEF: %d", option1->defense);
    mvwprintw(win, 9, 4, "Sp-ATK: %d", option1->specialAttack);
    mvwprintw(win, 10, 4, "Sp-DEF: %d", option1->specialDefense);
    mvwprintw(win, 11, 4, "SPD: %d", option1->speed);
    mvwprintw(win, 13, 4, "Knows:");
    mvwprintw(win, 14, 5, "%s", option1->pokeMoves[0].identifier);
    mvwprintw(win, 15, 5, "%s", option1->pokeMoves[1].identifier);
    
    mvwprintw(win, 2, 32, "%s", option2->name);
    mvwprintw(win, 3, 28, "Gender: %s", (option2->gender ? "Male" : "Female"));
    mvwprintw(win, 4, 28, "Level: %d", option2->level);
    mvwprintw(win, 6, 28, "HP: %d", option2->hp);
    mvwprintw(win, 7, 28, "ATK: %d", option2->attack);
    mvwprintw(win, 8, 28, "DEF: %d", option2->defense);
    mvwprintw(win, 9, 28, "Sp-ATK: %d", option2->specialAttack);
    mvwprintw(win, 10, 28, "Sp-DEF: %d", option2->specialDefense);
    mvwprintw(win, 11, 28, "SPD: %d", option2->speed);
    mvwprintw(win, 13, 28, "Knows:");
    mvwprintw(win, 14, 29, "%s", option2->pokeMoves[0].identifier);
    mvwprintw(win, 15, 29, "%s", option2->pokeMoves[1].identifier);

    mvwprintw(win, 2, 59, "%s", option3->name);
    mvwprintw(win, 3, 55, "Gender: %s", (option3->gender ? "Male" : "Female"));
    mvwprintw(win, 4, 55, "Level: %d", option3->level);
    mvwprintw(win, 6, 55, "HP: %d", option3->hp);
    mvwprintw(win, 7, 55, "ATK: %d", option3->attack);
    mvwprintw(win, 8, 55, "DEF: %d", option3->defense);
    mvwprintw(win, 9, 55, "Sp-ATK: %d", option3->specialAttack);
    mvwprintw(win, 10, 55, "Sp-DEF: %d", option3->specialDefense);
    mvwprintw(win, 11, 55, "SPD: %d", option3->speed);
    mvwprintw(win, 13, 55, "Knows:");
    mvwprintw(win, 14, 56, "%s", option3->pokeMoves[0].identifier);
    mvwprintw(win, 15, 56, "%s", option3->pokeMoves[1].identifier);

    echo();
    mvwprintw(win, 17, 10, "Please enter 1, 2, or 3 to make your selection: ");
    move(19, 59);
    wrefresh(win);
    char str[10] = "";
    while(strcmp(str, "") == 0) {
        getstr(str);
        if (strcmp(str, "1") == 0) { // Choose Option 1
            strcpy(heldPokemon[0].name, option1->name);

            // Copy moves
                heldPokemon[0].pokeMoves[0].id = option1->pokeMoves[0].id;
                strcpy(heldPokemon[0].pokeMoves[0].identifier, option1->pokeMoves[0].identifier);
                heldPokemon[0].pokeMoves[0].generation_id = option1->pokeMoves[0].generation_id;
                heldPokemon[0].pokeMoves[0].type_id = option1->pokeMoves[0].type_id;
                heldPokemon[0].pokeMoves[0].power = option1->pokeMoves[0].power;
                heldPokemon[0].pokeMoves[0].pp = option1->pokeMoves[0].pp;
                heldPokemon[0].pokeMoves[0].accuracy = option1->pokeMoves[0].accuracy;
                heldPokemon[0].pokeMoves[0].priority = option1->pokeMoves[0].priority;
                heldPokemon[0].pokeMoves[0].target_id = option1->pokeMoves[0].target_id;
                heldPokemon[0].pokeMoves[0].damage_class_id = option1->pokeMoves[0].damage_class_id;
                heldPokemon[0].pokeMoves[0].effect_id = option1->pokeMoves[0].effect_id;
                heldPokemon[0].pokeMoves[0].effect_chance = option1->pokeMoves[0].effect_chance;
                heldPokemon[0].pokeMoves[0].contest_type_id = option1->pokeMoves[0].contest_type_id;
                heldPokemon[0].pokeMoves[0].contest_effect_id = option1->pokeMoves[0].contest_effect_id;
                heldPokemon[0].pokeMoves[0].super_contest_effect_id = option1->pokeMoves[0].super_contest_effect_id;

                heldPokemon[0].pokeMoves[1].id = option1->pokeMoves[1].id;
                strcpy(heldPokemon[0].pokeMoves[1].identifier, option1->pokeMoves[1].identifier);
                heldPokemon[0].pokeMoves[1].generation_id = option1->pokeMoves[1].generation_id;
                heldPokemon[0].pokeMoves[1].type_id = option1->pokeMoves[1].type_id;
                heldPokemon[0].pokeMoves[1].power = option1->pokeMoves[1].power;
                heldPokemon[0].pokeMoves[1].pp = option1->pokeMoves[1].pp;
                heldPokemon[0].pokeMoves[1].accuracy = option1->pokeMoves[1].accuracy;
                heldPokemon[0].pokeMoves[1].priority = option1->pokeMoves[1].priority;
                heldPokemon[0].pokeMoves[1].target_id = option1->pokeMoves[1].target_id;
                heldPokemon[0].pokeMoves[1].damage_class_id = option1->pokeMoves[1].damage_class_id;
                heldPokemon[0].pokeMoves[1].effect_id = option1->pokeMoves[1].effect_id;
                heldPokemon[0].pokeMoves[1].effect_chance = option1->pokeMoves[1].effect_chance;
                heldPokemon[0].pokeMoves[1].contest_type_id = option1->pokeMoves[1].contest_type_id;
                heldPokemon[0].pokeMoves[1].contest_effect_id = option1->pokeMoves[1].contest_effect_id;
                heldPokemon[0].pokeMoves[1].super_contest_effect_id = option1->pokeMoves[1].super_contest_effect_id;

            
            heldPokemon[0].gender = option1->gender;
            heldPokemon[0].shiny = option1->shiny;
            heldPokemon[0].level = option1->level;
            for (int i = 0; i < 101; i++) {
                heldPokemon[0].levels[i].growth_rate_id = option1->levels[i].growth_rate_id;
                heldPokemon[0].levels[i].level = option1->levels[i].level;
                heldPokemon[0].levels[i].experience = option1->levels[i].experience;
            }
            heldPokemon[0].exp = option1->exp;
            heldPokemon[0].lvlRate = option1->lvlRate;
            heldPokemon[0].pokeId = option1->pokeId;
            heldPokemon[0].speciesId = option1->speciesId;
            heldPokemon[0].hp = option1->hp;
            heldPokemon[0].hpIV = option1->hpIV;
            heldPokemon[0].attack = option1->attack;
            heldPokemon[0].attackIV = option1->attackIV;
            heldPokemon[0].defense = option1->defense;
            heldPokemon[0].defenseIV = option1->defenseIV;
            heldPokemon[0].specialAttack = option1->specialAttack;
            heldPokemon[0].specialAttackIV = option1->specialAttackIV;
            heldPokemon[0].specialDefense = option1->specialDefense;
            heldPokemon[0].specialDefenseIV = option1->specialDefenseIV;
            heldPokemon[0].speed = option1->speed;
            heldPokemon[0].speedIV = option1->speedIV;
        } else if (strcmp(str, "2") == 0) { // Choose Option 2
            strcpy(heldPokemon[0].name, option2->name);

            // Copy moves
                heldPokemon[0].pokeMoves[0].id = option2->pokeMoves[0].id;
                strcpy(heldPokemon[0].pokeMoves[0].identifier, option2->pokeMoves[0].identifier);
                heldPokemon[0].pokeMoves[0].generation_id = option2->pokeMoves[0].generation_id;
                heldPokemon[0].pokeMoves[0].type_id = option2->pokeMoves[0].type_id;
                heldPokemon[0].pokeMoves[0].power = option2->pokeMoves[0].power;
                heldPokemon[0].pokeMoves[0].pp = option2->pokeMoves[0].pp;
                heldPokemon[0].pokeMoves[0].accuracy = option2->pokeMoves[0].accuracy;
                heldPokemon[0].pokeMoves[0].priority = option2->pokeMoves[0].priority;
                heldPokemon[0].pokeMoves[0].target_id = option2->pokeMoves[0].target_id;
                heldPokemon[0].pokeMoves[0].damage_class_id = option2->pokeMoves[0].damage_class_id;
                heldPokemon[0].pokeMoves[0].effect_id = option2->pokeMoves[0].effect_id;
                heldPokemon[0].pokeMoves[0].effect_chance = option2->pokeMoves[0].effect_chance;
                heldPokemon[0].pokeMoves[0].contest_type_id = option2->pokeMoves[0].contest_type_id;
                heldPokemon[0].pokeMoves[0].contest_effect_id = option2->pokeMoves[0].contest_effect_id;
                heldPokemon[0].pokeMoves[0].super_contest_effect_id = option2->pokeMoves[0].super_contest_effect_id;

                heldPokemon[0].pokeMoves[1].id = option2->pokeMoves[1].id;
                strcpy(heldPokemon[0].pokeMoves[1].identifier, option2->pokeMoves[1].identifier);
                heldPokemon[0].pokeMoves[1].generation_id = option2->pokeMoves[1].generation_id;
                heldPokemon[0].pokeMoves[1].type_id = option2->pokeMoves[1].type_id;
                heldPokemon[0].pokeMoves[1].power = option2->pokeMoves[1].power;
                heldPokemon[0].pokeMoves[1].pp = option2->pokeMoves[1].pp;
                heldPokemon[0].pokeMoves[1].accuracy = option2->pokeMoves[1].accuracy;
                heldPokemon[0].pokeMoves[1].priority = option2->pokeMoves[1].priority;
                heldPokemon[0].pokeMoves[1].target_id = option2->pokeMoves[1].target_id;
                heldPokemon[0].pokeMoves[1].damage_class_id = option2->pokeMoves[1].damage_class_id;
                heldPokemon[0].pokeMoves[1].effect_id = option2->pokeMoves[1].effect_id;
                heldPokemon[0].pokeMoves[1].effect_chance = option2->pokeMoves[1].effect_chance;
                heldPokemon[0].pokeMoves[1].contest_type_id = option2->pokeMoves[1].contest_type_id;
                heldPokemon[0].pokeMoves[1].contest_effect_id = option2->pokeMoves[1].contest_effect_id;
                heldPokemon[0].pokeMoves[1].super_contest_effect_id = option2->pokeMoves[1].super_contest_effect_id;

            
            heldPokemon[0].gender = option2->gender;
            heldPokemon[0].shiny = option2->shiny;
            heldPokemon[0].level = option2->level;
            for (int i = 0; i < 101; i++) {
                heldPokemon[0].levels[i].growth_rate_id = option2->levels[i].growth_rate_id;
                heldPokemon[0].levels[i].level = option2->levels[i].level;
                heldPokemon[0].levels[i].experience = option2->levels[i].experience;
            }
            heldPokemon[0].exp = option2->exp;
            heldPokemon[0].lvlRate = option2->lvlRate;
            heldPokemon[0].pokeId = option2->pokeId;
            heldPokemon[0].speciesId = option2->speciesId;
            heldPokemon[0].hp = option2->hp;
            heldPokemon[0].hpIV = option2->hpIV;
            heldPokemon[0].attack = option2->attack;
            heldPokemon[0].attackIV = option2->attackIV;
            heldPokemon[0].defense = option2->defense;
            heldPokemon[0].defenseIV = option2->defenseIV;
            heldPokemon[0].specialAttack = option2->specialAttack;
            heldPokemon[0].specialAttackIV = option2->specialAttackIV;
            heldPokemon[0].specialDefense = option2->specialDefense;
            heldPokemon[0].specialDefenseIV = option2->specialDefenseIV;
            heldPokemon[0].speed = option2->speed;
            heldPokemon[0].speedIV = option2->speedIV;
        } else if (strcmp(str, "3") == 0) { // Choose Option 3
            strcpy(heldPokemon[0].name, option3->name);

            // Copy moves
                heldPokemon[0].pokeMoves[0].id = option3->pokeMoves[0].id;
                strcpy(heldPokemon[0].pokeMoves[0].identifier, option3->pokeMoves[0].identifier);
                heldPokemon[0].pokeMoves[0].generation_id = option3->pokeMoves[0].generation_id;
                heldPokemon[0].pokeMoves[0].type_id = option3->pokeMoves[0].type_id;
                heldPokemon[0].pokeMoves[0].power = option3->pokeMoves[0].power;
                heldPokemon[0].pokeMoves[0].pp = option3->pokeMoves[0].pp;
                heldPokemon[0].pokeMoves[0].accuracy = option3->pokeMoves[0].accuracy;
                heldPokemon[0].pokeMoves[0].priority = option3->pokeMoves[0].priority;
                heldPokemon[0].pokeMoves[0].target_id = option3->pokeMoves[0].target_id;
                heldPokemon[0].pokeMoves[0].damage_class_id = option3->pokeMoves[0].damage_class_id;
                heldPokemon[0].pokeMoves[0].effect_id = option3->pokeMoves[0].effect_id;
                heldPokemon[0].pokeMoves[0].effect_chance = option3->pokeMoves[0].effect_chance;
                heldPokemon[0].pokeMoves[0].contest_type_id = option3->pokeMoves[0].contest_type_id;
                heldPokemon[0].pokeMoves[0].contest_effect_id = option3->pokeMoves[0].contest_effect_id;
                heldPokemon[0].pokeMoves[0].super_contest_effect_id = option3->pokeMoves[0].super_contest_effect_id;

                heldPokemon[0].pokeMoves[1].id = option3->pokeMoves[1].id;
                strcpy(heldPokemon[0].pokeMoves[1].identifier, option3->pokeMoves[1].identifier);
                heldPokemon[0].pokeMoves[1].generation_id = option3->pokeMoves[1].generation_id;
                heldPokemon[0].pokeMoves[1].type_id = option3->pokeMoves[1].type_id;
                heldPokemon[0].pokeMoves[1].power = option3->pokeMoves[1].power;
                heldPokemon[0].pokeMoves[1].pp = option3->pokeMoves[1].pp;
                heldPokemon[0].pokeMoves[1].accuracy = option3->pokeMoves[1].accuracy;
                heldPokemon[0].pokeMoves[1].priority = option3->pokeMoves[1].priority;
                heldPokemon[0].pokeMoves[1].target_id = option3->pokeMoves[1].target_id;
                heldPokemon[0].pokeMoves[1].damage_class_id = option3->pokeMoves[1].damage_class_id;
                heldPokemon[0].pokeMoves[1].effect_id = option3->pokeMoves[1].effect_id;
                heldPokemon[0].pokeMoves[1].effect_chance = option3->pokeMoves[1].effect_chance;
                heldPokemon[0].pokeMoves[1].contest_type_id = option3->pokeMoves[1].contest_type_id;
                heldPokemon[0].pokeMoves[1].contest_effect_id = option3->pokeMoves[1].contest_effect_id;
                heldPokemon[0].pokeMoves[1].super_contest_effect_id = option3->pokeMoves[1].super_contest_effect_id;

            
            heldPokemon[0].gender = option3->gender;
            heldPokemon[0].shiny = option3->shiny;
            heldPokemon[0].level = option3->level;
            for (int i = 0; i < 101; i++) {
                heldPokemon[0].levels[i].growth_rate_id = option3->levels[i].growth_rate_id;
                heldPokemon[0].levels[i].level = option3->levels[i].level;
                heldPokemon[0].levels[i].experience = option3->levels[i].experience;
            }
            heldPokemon[0].exp = option3->exp;
            heldPokemon[0].lvlRate = option3->lvlRate;
            heldPokemon[0].pokeId = option3->pokeId;
            heldPokemon[0].speciesId = option3->speciesId;
            heldPokemon[0].hp = option3->hp;
            heldPokemon[0].hpIV = option3->hpIV;
            heldPokemon[0].attack = option3->attack;
            heldPokemon[0].attackIV = option3->attackIV;
            heldPokemon[0].defense = option3->defense;
            heldPokemon[0].defenseIV = option3->defenseIV;
            heldPokemon[0].specialAttack = option3->specialAttack;
            heldPokemon[0].specialAttackIV = option3->specialAttackIV;
            heldPokemon[0].specialDefense = option3->specialDefense;
            heldPokemon[0].specialDefenseIV = option3->specialDefenseIV;
            heldPokemon[0].speed = option3->speed;
            heldPokemon[0].speedIV = option3->speedIV;
        } else {
            strcpy(str, "");
            mvwprintw(win, 17, 10, "%-67s", "Please enter 1, 2, or 3 to make your selection: ");
            move(19, 59);
        }
        wrefresh(win);
    }
    
    werase(win);
    box(win, 0, 0);

    mvwprintw(win, 8, 25, "Good luck!");
    mvwprintw(win, 17, 48, "Press any key to continue...");
    wrefresh(win);

    refresh();
    noecho();
}

void pokemonBattle(character *trainer) {
    mvprintw(22, 5, "You have a %s!", heldPokemon[0].name);

    mvprintw(24, 3, "HP: %d", heldPokemon[0].hp);
    mvprintw(25, 3, "ATK: %d", heldPokemon[0].attack);
    mvprintw(26, 3, "DEF: %d", heldPokemon[0].defense);
    mvprintw(27, 3, "Sp-ATK: %d", heldPokemon[0].specialAttack);
    mvprintw(28, 3, "Sp-DEF: %d", heldPokemon[0].specialDefense);
    mvprintw(29, 3, "SPD: %d", heldPokemon[0].speed);
    mvprintw(30, 3, "Knows:");
    mvprintw(31, 4, "%s", heldPokemon[0].pokeMoves[0].identifier);
    mvprintw(32, 4, "%s", heldPokemon[0].pokeMoves[1].identifier);
    
    mvprintw(24, 16, "Gender: %s", (heldPokemon[0].gender ? "Male" : "Female"));
    mvprintw(25, 16, "Shiny: %s", (heldPokemon[0].shiny ? "Yes" : "No"));
    mvprintw(26, 16, "Level: %d", heldPokemon[0].level);
    mvprintw(27, 16, "Exp: %d", heldPokemon[0].exp);

    int pokemonToFight = rand() % trainer->numPokemon;

    mvprintw(22, 33, "Trainer %c has a %s!", trainer->npc, trainer->heldPokemon[pokemonToFight].name);

    mvprintw(24, 31, "HP: %d", trainer->heldPokemon[pokemonToFight].hp);
    mvprintw(25, 31, "ATK: %d", trainer->heldPokemon[pokemonToFight].attack);
    mvprintw(26, 31, "DEF: %d", trainer->heldPokemon[pokemonToFight].defense);
    mvprintw(27, 31, "Sp-ATK: %d", trainer->heldPokemon[pokemonToFight].specialAttack);
    mvprintw(28, 31, "Sp-DEF: %d", trainer->heldPokemon[pokemonToFight].specialDefense);
    mvprintw(29, 31, "SPD: %d", trainer->heldPokemon[pokemonToFight].speed);
    mvprintw(30, 31, "Knows:");
    mvprintw(31, 32, "%s", trainer->heldPokemon[pokemonToFight].pokeMoves[pokemonToFight].identifier);
    mvprintw(32, 32, "%s", trainer->heldPokemon[pokemonToFight].pokeMoves[1].identifier);
    
    mvprintw(24, 44, "Gender: %s", (trainer->heldPokemon[pokemonToFight].gender ? "Male" : "Female"));
    mvprintw(25, 44, "Shiny: %s", (trainer->heldPokemon[pokemonToFight].shiny ? "Yes" : "No"));
    mvprintw(26, 44, "Level: %d", trainer->heldPokemon[pokemonToFight].level);
    mvprintw(27, 44, "Exp: %d", trainer->heldPokemon[pokemonToFight].exp);
    mvprintw(28, 44, "NP: %d", trainer->numPokemon);

    refresh();
}

int encounterPokemon() {
    // generate a pokemon                       
    // int manhattan = abs(0 - terrainMap->worldRow) - abs(0 - terrainMap->worldCol);
    worldPokemon *p = static_cast<worldPokemon*>(malloc(sizeof(worldPokemon)));
    createPokemon(p);

    WINDOW *win = newwin(14, 33, 3, 5);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, "You found a %s!", p->name);

    mvwprintw(win, 2, 3, "HP: %d", p->hp);
    mvwprintw(win, 3, 3, "ATK: %d", p->attack);
    mvwprintw(win, 4, 3, "DEF: %d", p->defense);
    mvwprintw(win, 5, 3, "Sp-ATK: %d", p->specialAttack);
    mvwprintw(win, 6, 3, "Sp-DEF: %d", p->specialDefense);
    mvwprintw(win, 7, 3, "SPD: %d", p->speed);
    mvwprintw(win, 9, 3, "Knows:");
    mvwprintw(win, 10, 5, "%s", p->pokeMoves[0].identifier);
    mvwprintw(win, 11, 5, "%s", p->pokeMoves[1].identifier);
    
    mvwprintw(win, 2, 16, "Gender: %s", (p->gender ? "Male" : "Female"));
    mvwprintw(win, 3, 16, "Shiny: %s", (p->shiny ? "Yes" : "No"));
    mvwprintw(win, 5, 16, "Level: %d", p->level);
    mvwprintw(win, 6, 16, "Exp: %d", p->exp);

    mvwprintw(win, 13, 2, "Press any key to exit");
    wrefresh(win);
    refresh();

    usleep(1000000);

    int c = getch();

    free(p);
    return c;
}