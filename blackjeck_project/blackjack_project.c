#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h> 

struct cards {
    uint8_t data;
    struct cards* next_card;
};

struct CardList {
    struct cards* head;
    size_t len;
};

// Function signatures
void add_to_list(struct CardList* list, struct cards* new_card);  //done
void initialize_deck(struct CardList* deck); 
int  handle_betting(int* cash, int* pot);// done
void initial_deal(struct CardList* deck, struct CardList* player, struct CardList* dealer);// done
int  get_suit_index(uint8_t data);// done
void move_random_card(struct CardList* from, struct CardList* to); // done
int  calculate_hand_value(struct CardList* list);// done
int  check_black_jack(struct CardList* player, int* cash, int* pot);// done
int  handle_hit_or_stand(struct CardList* deck, struct CardList* player, int* pot);// done
void free_list(struct CardList* list); //done
void return_cards_to_deck(struct CardList* deck, struct CardList* player,struct CardList* dealer); //done
struct cards* get_last_card(struct CardList* list);  // done
int handle_dealer_draw(struct CardList* deck, struct CardList* dealer,int player_score,int* cash, int* pot); // done
void print_full_hand(struct CardList* hand); // done                     
                          

//Constant naming for clarity

//   represents Suitbits =    0001      0010        0100      1000   
const char* SUIT_NAMES[] = { "Clubs", "Diamonds", "Hearts", "Spades" };

//   represents Rankbits =    0000    0001  0010 0011 0100 0101 0110 0111 1000 1001 1010   1011    1100     1101
const char* RANK_NAMES[] = { "None", "Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King" };



//////////////////////////////////// M-A-I-N ///////////////////////////////////////

int main(void){

    int player_cash = 1000; // Starting money
    int pot = 0;            // Starts at 0
    char play_again[10];
           
    srand(time(NULL)); 

    puts("\033[2J\033[H");

    // 1. Setup lists

    struct CardList shared_deck = { NULL, 0 };
    struct CardList player_hand = { NULL, 0 };
    struct CardList dealer_hand = { NULL, 0 };


    // 2. Start Game
    initialize_deck(&shared_deck);
    printf("Deck initialized. Size: %zu cards\n", shared_deck.len);
     


    // Game Loop
    while (1) {
        if (!handle_betting(&player_cash, &pot)) { // handle_betting returns 0 if player broke, 1 if game continue
            free_list(&shared_deck);               // Free  
            free_list(&player_hand);               //       allocated memory 
            free_list(&dealer_hand);               //                         before exiting
            break;                  
        }

        // Proceed to Initial Deal Phase...
        printf("Dealing cards...\n");
        puts("");
        initial_deal(&shared_deck, &player_hand, &dealer_hand);

        if (check_black_jack(&player_hand, &player_cash, &pot)) { //check_black_jack returns 1 if player got Black Jack (End Round), 0 otherwise (Continue to Hit/Stand)
            return_cards_to_deck(&shared_deck, &player_hand, &dealer_hand);//(moving cards back to deck)
            continue;
        }
        // Player Hit/Stand phase
    int player_alive=handle_hit_or_stand(&shared_deck, &player_hand, &pot);

    if(!player_alive){
        //player busted, dealer wins
        printf("Dealer wins.\n");
        pot=0;
        return_cards_to_deck(&shared_deck, &player_hand, &dealer_hand);
        continue;
    }
    // Dealer Drew Phase
    int player_score=calculate_hand_value(&player_hand);
    int dealer_resault=handle_dealer_draw(&shared_deck,&dealer_hand,player_score,&player_cash,&pot);
    //dealer_resault:
    //0=dealer wins
    //1=player wins
    //2=tie

    // Reset Cards
    return_cards_to_deck(&shared_deck, &player_hand, &dealer_hand);

    // Asking player if he wants to continue
    printf("\nWould you like to play again? (yes/no): ");
    scanf("%9s", play_again);

    // If player chooses no → exit game
    if (strcmp(play_again, "no") == 0 || strcmp(play_again, "No") == 0) {
    printf("Thanks for playing! Goodbye.\n");
    free_list(&shared_deck);
    free_list(&player_hand);
    free_list(&dealer_hand);

    break;
}

    // If player has no money → exit game
    if (player_cash < 10 && pot == 0) {
    printf("You don't have enough money to continue. Game Over!\n");
    free_list(&shared_deck);
    free_list(&player_hand);
    free_list(&dealer_hand);

    break;
    
    // Otherwise loop continues automatically
}



  }

} 
    
////////////////////////////////////////////////////////////////////////////////

// Function to add a card to the front of a list 
void add_to_list(struct CardList* list, struct cards* new_card) {
    if (!new_card) return;
    new_card->next_card = list->head;
    list->head = new_card;
    list->len++;
}

// Function to initialize the shared deck with all 52 cards
void initialize_deck(struct CardList* deck) {
    
    for (uint8_t s = 0; s < 4; s++) {
        uint8_t suit_bits = (uint8_t)(1 << s); // suit_bits will be equal=1000,0100,0010,0001
        for ( uint8_t r = 1; r <= 13; r++) {   // 1000=Spades, 0100=Hearts, 0010=Dimonds, 0001=Clubs
            struct cards* new_card = malloc(sizeof(struct cards));
            if (new_card) {
                new_card->data = (r << 4) | (suit_bits & 0x0F); // we can skip &0x0F it won't change the resault 
                add_to_list(deck, new_card);                    // using it just for the protocol to show usage of mask 
            }
        }
    }
}

// Returns 1 if the game continues, 0 if player is broke (Game Over) and updates pot and players cash
int handle_betting(int* cash, int* pot) {
    printf("\n--- Betting Phase ---\n");
    printf("Your Cash: %d | Current Pot: %d\n", *cash, *pot);

    // 1. Check for Game Over
    if (*pot == 0 && *cash < 10) {
        printf("You have less than 10$ and the pot is empty. Game Over!\n");
        return 0;
    }

    int bet = 0;
    int valid_bet = 0;

    // 2. Betting Loop
    while (!valid_bet) {
        printf("How much would you like to add to the pot? (Multiples of 10): ");
        if (scanf("%d", &bet) != 1) {
            // Clear input buffer if user enters non-numeric value
            while (getchar() != '\n');
            printf("Invalid input. Please enter a  number.\n");
            continue;
        }
    

        if (bet < 0) {
            printf("Bet cannot be negative or.\n");
        } else if (bet > *cash) {
            printf("You don't have enough cash!\n");
        } else if (bet % 10 != 0) {
            printf("Bets must be in multiples of 10.\n");
        } else if (bet == 0 && *pot == 0) {
            printf("You must bet at least 10 to start the pot.\n");
        } else {
            valid_bet = 1;
        }
    }

    // 3. Update values
    *cash -= bet;
    *pot += bet;

    printf("New Balance - Cash: %d | Pot: %d\n", *cash, *pot);
    return 1;
  }

                           //////////////////////  
/////////////////////////// F i r s t- D e a l ///////////////////////////////////////////
                         //////////////////////
void initial_deal(struct CardList* deck, struct CardList* player, struct CardList* dealer) {
    // 1. Deal 2 cards to each
    for (int i = 0; i < 2; i++) {
        move_random_card(deck, player);
        move_random_card(deck, dealer);
    }

    // Printing Dealer's hand ( second card hidden)}
    // 2. Output Dealer's cards
    printf("dealer: ");
    struct cards* d_temp = (*dealer).head;
    
    //  showing the first card's name
    uint8_t d_rank = (*d_temp).data >> 4;
    int d_suit_idx = get_suit_index((*d_temp).data);
    printf("%s of %s, ????????\n", RANK_NAMES[d_rank], SUIT_NAMES[d_suit_idx]);

    // 3. Output Player's cards using a simple loop
    printf("player: ");
    struct cards* p_temp = (*player).head;
    for (size_t i = 0; i < (*player).len; i++) {
        uint8_t p_rank = (*p_temp).data >> 4;
        int p_suit_idx = get_suit_index((*p_temp).data);
        
        printf("%s of %s", RANK_NAMES[p_rank], SUIT_NAMES[p_suit_idx]);
        
        // Add a comma if it's not the last card
        if (i < (*player).len - 1) {
            printf(", ");
        }
        
        p_temp = (*p_temp).next_card; // Move to next card
    }
    printf("\n");
}

// Helper to convert bit pattern (1,2,4,8) to array index (0,1,2,3)
int get_suit_index(uint8_t data) {
    uint8_t suit_bits = data & 0x0F;
    if (suit_bits == 1) return 0; // Clubs-(0001)
    if (suit_bits == 2) return 1; // Diamonds-(0010)
    if (suit_bits == 4) return 2; // Hearts-(0100)
    if (suit_bits == 8) return 3; // Spades-(1000)
}

// Helper to remove from 'from' at index and add to the end of 'to'
void move_random_card(struct CardList* from, struct CardList* to) {
    if (from->len == 0) return;

    // 1. Pick a random index
    int index = rand() % from->len;

    struct cards* prev = NULL;
    struct cards* target = from->head;

    // 2. Navigate to the selected index
    for (int i = 0; i < index; i++) {
        prev = target;
        target = target->next_card;
    }

    // 3. Remove from 'from' list
    if (prev == NULL) {
        from->head = target->next_card; // Target was the head
    } else {
        prev->next_card = target->next_card;
    }
    from->len--;

    // 4. Prepare target to be the NEW tail
    target->next_card = NULL;

    // 5. Insert at the LAST position of 'to'
    if (to->head == NULL) {
        to->head = target;
    } else {
        struct cards* last = to->head;
        while (last->next_card != NULL) {
            last = last->next_card;
        }
        last->next_card = target;
    }
    to->len++;
}

int calculate_hand_value(struct CardList* list) {
    int total = 0;
    int has_ace = 0;
    struct cards* temp = (*list).head;

    while (temp != NULL) {
        uint8_t rank = (*temp).data >> 4; // Extract rank bits

        if (rank >= 11) {
            // Jack, Queen, King
            total += 10;
        } else if (rank == 1) {
            // Ace
            total += 1;
            has_ace = 1;
        } else {
            // 2 through 10
            total += rank;
        }
        temp = (*temp).next_card;
    }

    // Ace logic: If we have an Ace and adding 10 in total less than 21
    if (has_ace && (total + 10 <= 21)) {
        total += 10;
    }

    return total;
}

// Returns 1 if player got Black Jack (End Round), 0 otherwise (Continue to Hit/Stand)
int check_black_jack(struct CardList* player, int* cash, int* pot) {
    int score = calculate_hand_value(player);

    if (score == 21) {
        printf("Black Jack!\n");

        // Payout: original pot + 1.5 * pot
        // We use 3/2 or * 1.5 for the bonus calculation
        int winnings = *pot + (*pot * 1.5);
        *cash += winnings;
        *pot = 0;

        printf("You won %d! New balance: %d\n", winnings, *cash);
        return 1; // Move to Reset Cards phase
    }

    return 0; // Move to Hit or Stand phase
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//                        H-I-T                                                                  //
//                               O-R                                                             //
//                                    S-T-A-N-D                                                  // 
/////////////////////////////////////////////////////////////////////////////////////////////////// 

// Returns 1 if player is still in the game and choose stand, 0 if player busted
int handle_hit_or_stand(struct CardList* deck, struct CardList* player, int* pot) {
    char choice[10];
    int score;

    while (1) {
        score = calculate_hand_value(player);
        printf("\nYour current score: %d\n", score);
        printf("Choose 'hit' to draw a card or 'stand' to stop: ");
        
        scanf("%9s", choice); // Limits input to 9 characters to prevent overflow

        // 1. If player chooses Stand
        if (strcmp(choice, "stand") == 0) {
            return 1; // Move to Dealer Draw Phase
        }

        // 2. If player chooses Hit
        if (strcmp(choice, "hit") == 0) {
             // Draw a card and append it to player's hand
            move_random_card(deck, player);
            // Get the newly added card from players list
            struct cards* new_card_from_deck = get_last_card(player);
            uint8_t r=((*new_card_from_deck).data)>>4;
            int s_idx = get_suit_index((*new_card_from_deck).data);
            printf("You drew: %s of %s\n", RANK_NAMES[r], SUIT_NAMES[s_idx]);

            // Check for Bust
            score = calculate_hand_value(player);
            if (score > 21) {
                printf("Bust! Your score is %d. You lose the bet.\n", score);
                *pot = 0;
                return 0; 
            }
        } else {
            printf("Invalid input. Please type 'hit' or 'stand'.\n");
        }
    }
}

struct cards* get_last_card(struct CardList* list) {
    if (list->head == NULL) return NULL;

    struct cards* current = list->head;
    while (current->next_card != NULL) {
        current = current->next_card;
    }
    return current;
}

////////////////////////////////// FREE MEMORY FUNCTION /////////////////////////////////////

void free_list(struct CardList* list) {
    struct cards* current = list->head;
    while (current != NULL) {
        struct cards* next = current->next_card;
        free(current);
        current = next;
    }
    list->head = NULL;
    list->len = 0;
}

/////////////////////////////////// Return Cards to Deck ////////////////////////////////////

void return_cards_to_deck(struct CardList* deck,
                          struct CardList* player,
                          struct CardList* dealer)
{
    struct cards* current;

    // Move all cards from player hand back to deck
    current = player->head;
    while (current != NULL) {
        struct cards* next = current->next_card;

        // Insert at deck head
        current->next_card = deck->head;
        deck->head = current;
        deck->len++;

        current = next;
    }
    player->head = NULL;
    player->len = 0;

    // Move all cards from dealer hand back to deck
    current = dealer->head;
    while (current != NULL) {
        struct cards* next = current->next_card;

        // Insert at deck head
        current->next_card = deck->head;
        deck->head = current;
        deck->len++;

        current = next;
    }
    dealer->head = NULL;
    dealer->len = 0;
}

///////////////////////////////// D-E-A-L-E-R  T-U-R-N /////////////////////////////////

int handle_dealer_draw(struct CardList* deck,
                       struct CardList* dealer,
                       int player_score,
                       int* cash,
                       int* pot)
{
    printf("\n--- Dealer Draw Phase ---\n");

    int dealer_score = calculate_hand_value(dealer);
    printf("Dealer reveals hand (score %d)\n", dealer_score);
    print_full_hand(dealer);

    // 1. Dealer has 21 on reveal
    if (dealer_score == 21) {
        if (player_score == 21) {
            printf("Tie.\n");
            return 2;
        } else {
            printf("Dealer wins.\n");
            *pot = 0;
            return 0;
        }
    }

    // 2. Dealer already beats the player
    if (dealer_score > player_score) {
        printf("Dealer wins.\n");
        *pot = 0;
        return 0;
    }

    // 3. Dealer draws
    while (dealer_score < 17 && dealer_score <= player_score) {
        printf("Dealer hits...\n");
        move_random_card(deck, dealer);

        struct cards* new_card = get_last_card(dealer);
        uint8_t r = new_card->data >> 4;
        int s_idx = get_suit_index(new_card->data);
        printf("Dealer drew: %s of %s\n", RANK_NAMES[r], SUIT_NAMES[s_idx]);

        dealer_score = calculate_hand_value(dealer);
        printf("Dealer score is now %d\n", dealer_score);
    }

    // 4. Dealer hits into 21
    if (dealer_score == 21) {
        if (player_score == 21) {
            printf("Tie.\n");
            return 2;
        } else {
            printf("Dealer wins.\n");
            *pot = 0;
            return 0;
        }
    }

    // 5. Dealer busts
    if (dealer_score > 21) {
        printf("Dealer bust!\n");
        *cash += (*pot * 2);
        *pot = 0;
        return 1;
    }

    // 6. Final comparison
    if (dealer_score == player_score) {
        printf("Tie.\n");
        return 2;
    }

    if (dealer_score > player_score) {
        printf("Dealer wins.\n");
        *pot = 0;
        return 0;
    }

    printf("Player wins!\n");
    *cash += (*pot * 2);
    *pot = 0;
    return 1;
}





void print_full_hand(struct CardList* hand) {
    struct cards* temp = hand->head;

    while (temp != NULL) {
        uint8_t rank = temp->data >> 4;
        int suit_idx = get_suit_index(temp->data);

        printf("%s of %s", RANK_NAMES[rank], SUIT_NAMES[suit_idx]);

        if (temp->next_card != NULL)
            printf(", ");

        temp = temp->next_card;
    }
    printf("\n");
}

