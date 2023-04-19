# ComS327-Battles (1.09)
### Author:
Ethan Hancock, ehancock@iastate.edu
### TA Notes: 
> 

### Description:
#### Major Changes:
 - Wild Pokemon and Trainers are now able to be engaged in battle
 - Player now has an inventory and supplies (pokeballs, revives, & potions)
 - Supplies are restorable via entering a Pokemart
 - Player's held pokemon are healable via entering a Pokecenter

#### Method Changes/Additions
 - Added battleSequence(), attackSequence(), & attackPlayer()
    - battleSequence() handles any battle with a pokemon
    - attackSequence() handles when the player chooses to attack the enemy pokemon
    - attackPlayer() is a method for the enemy pokemon to attack the player
        - Trainers always just attack every turn, they don't use items or do anything else, if their pokemon dies, the next is immediately put into battle
 - Refactored pokemonBattle() to force player to fight all of the trainers pokemon
 - Refactored some things in generateTrainers() for battling trainers
