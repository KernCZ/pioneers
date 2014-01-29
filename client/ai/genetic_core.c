#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_SIMS 1000
/*Number of simulations, could be raised to achieve better accuracy if computing time allows it*/
#define NUM_ACTIONS 20
/*Number of possible actions, single or paired -> 5 individual (SET,CIT,DEV, RSET RRSET) + 5*5 combined (SET+CIT,SET+DEV,etc) -10 of which are redundant (SET+CIT=CIT+SET) = 20*/
#define MAX_TURNS 100


enum action { SET, CIT, DEV, RSET, RRSET };	/*Possible individual actions: Settlement, City, Development Card, Road+Settlement, Road+Road+Settlement */
typedef enum action strategy_t[2];	/*Pair of preferred actions to carry out in the next turns, possibly the first (or even the second) right now */
typedef int tradingMatrix_t[5][5];	/*If it is set to 1 it means that I am interested in trading i resource in exchange of j resource */
int resourcesNeededForAction[NUM_ACTIONS][5] = {
	/*Resources of every type needed to perform every action possible */
	{1, 1, 1, 1, 0}, /*SET*/ 
	{0, 0, 2, 0, 3}, /*CIT*/ 
	{0, 0, 1, 1, 1}, /*DEV*/ 
	{2, 2, 1, 1, 0}, /*RSET*/ 
	{3, 3, 1, 1, 0}, /*RRSET*/ 
	
	{2, 2, 2, 2, 0},	/*SET+SET */
	{1, 1, 3, 1, 3},	/*SET+CIT */
	{1, 1, 2, 2, 1},	/*SET+DEV */
	{3, 3, 2, 2, 0},	/*SET+RSET */
	{4, 4, 2, 2, 0},	/*SET+RRSET */

	{0, 0, 4, 0, 6},	/*CIT+CIT */
	{0, 0, 3, 1, 4},	/*CIT+DEV */
	{2, 2, 3, 1, 3},	/*CIT+RSET */
	{3, 3, 3, 1, 3},	/*CIT+RRSET */

	{0, 0, 2, 2, 2},	/*DEV+DEV */
	{2, 2, 2, 2, 1},	/*DEV+RSET */
	{3, 3, 2, 2, 1},	/*DEV+RRSET */

	{4, 4, 2, 2, 0},	/*RSET+RSET */
	{5, 5, 2, 2, 0},	/*RSET+RRSET */

	{6, 6, 2, 2, 0}/*RRSET+RRSET */
};				/*First index is the action, second index the kind of resource */

struct tradingMatrixes_t {
	tradingMatrix_t internalTrade;
	tradingMatrix_t bankTrade;
	/*tradingMatrix_t maritimeTrade; */
};

/*A structure of type simulationsData will be used to hold the data of the MAX_SIMS simulations*/
struct simulationsData_t {
	int resourcesPool[MAX_SIMS][5];	/*Resources for every simulation */
	int conditionsMet[MAX_SIMS][NUM_ACTIONS];
	/*Boolean matrix that will hold true for every simulation that meets the requirements of certain action
	 * Possible actions are SET,CIT,DEV,RSET,RRSET, SET+SET,SET+CIT,SET+DEV...,CIT+SET,CIT+CIT,...RRSET+RSET,RRSET+RRSET
	 * 5 independent actions+15 combined pairs of actions.
	 * It is important to note that the order in which actions are represented in conditionsMet should match the order they are in resourcesNeededForAction*/
	int numberOfSimulationsOK[NUM_ACTIONS];	/*Number of simulations that meet the requirementS for every action */
	int turnsToAction[NUM_ACTIONS];	/*Number of turns needed for every action or pair of actions to reach the required probability of getting its resources */
	int timeCombinedAction[5][5];	/*It will hold the data of turnsToAction regarding combined actions, it is for ease of access, this information is already hold in turnsToAction */
};

/*A structure of type gameState_t will hold the information of my actual state in the game
 *resourcesSupply, resourcesAlreadyHave and actionValue are variables whose value depends on the actual situation of the player in the game*/

struct gameState_t {
	int resourcesSupply[11][5];	/*Depends on the location of settlements and cities in the map, 11 different dice outcomes by 5 different resources */
	int resourcesAlreadyHave[5];	/*Resources I already have of Brick, Lumber, Grain, Wool and Ore */
	float actionValue[5];	/*Benefit of doing best SET, best CIT, pick DEV, best RSET and best RRSET */
};

void printAction(enum action oneAction)
{
	switch (oneAction) {
	case 0:
		printf("Settlement");
		break;
	case 1:
		printf("City");
		break;
	case 2:
		printf("Development Card");
		break;
	case 3:
		printf("Road to Settlement");
		break;
	case 4:
		printf("Long Road to Settlement");
		break;
	}
}

void printResource(int resource)
{
	switch (resource) {
	case 0:
		printf("Brick");
		break;
	case 1:
		printf("Lumber");
		break;
	case 2:
		printf("Grain");
		break;
	case 3:
		printf("Wool");
		break;
	case 4:
		printf("Ore");
		break;
	}
}

int enoughResources(int sim, int act, struct simulationsData_t *Data)
{
	/*Returns TRUE if there are enough resources in simulation sim to perform action action */
	int resource;
	for (resource = 0; resource < 5; resource++) {	/*Check for every type of resource */
		if (Data->resourcesPool[sim][resource] <
		    resourcesNeededForAction[act][resource])
			return (0);
	}
	return (1);
}

void updateSimulation(int sim, float probability,
		      struct simulationsData_t *Data)
{
	/*Checks resourcesPool to update conditionsMet for that simulation for every action and numberOfSimulationsOK */
	int act;
	for (act = 0; act < NUM_ACTIONS; act++) {
		if ((Data->numberOfSimulationsOK[act] <
		     (MAX_SIMS * probability))
		    && (Data->conditionsMet[sim][act] != 1)) {
			/*If there already are enough simulations OK for this action or conditions were already met it doesnt need to check again */
			if (enoughResources(sim, act, Data)) {
				Data->conditionsMet[sim][act] = 1;
				Data->numberOfSimulationsOK[act]++;
			}
		}
	}
}

void updateConditionsMet(float probability, struct simulationsData_t *Data)
{
	/*Update every simulation in conditionsMet */
	/*We need probability in order to avoid checking for actions unnecessarily */
	int sim;
	for (sim = 0; sim < MAX_SIMS; sim++) {
		updateSimulation(sim, probability, Data);
	}
	/*At this point numberOfSimulationsOK will hold, for avery action, the number of simulations that, up to the turn calculated, meet the requirements to do it */
}

void updateTurnsToAction(float probability, int currentTurn,
			 struct simulationsData_t *Data)
{
	/*If the percentage of simulations OK for an action is over probability will set turnsToAction for that action to turn */
	int act;
	for (act = 0; act < NUM_ACTIONS; act++) {
		if ((Data->turnsToAction[act] == MAX_TURNS) &&	/*It will set only the first time it reaches probability */
		    (Data->numberOfSimulationsOK[act] >=
		     MAX_SIMS * probability))
			Data->turnsToAction[act] = currentTurn;
	}
}

void outputSims(int number, int turn, struct simulationsData_t *Data)
{
	int resource, act, simulation;

	system("clear");
	printf
	    ("BRICK\tLUMBER\tGRAIN\tWOOL\tORE\t\tSET\tCIT\tDEV\tRSET\tRRSET\tS+SET\tS+CIT\tS+DEV\tS+RSET\tS+RRSET\tC+CIT\tC+DEV\tC+RSET\tC+RRSET\tD+DEV\tD+RSET\tD+RRSET\tR+RSET\tR+RRSET\tRR+RRSET\n");
	for (simulation = 0; simulation < number; simulation++) {
		for (resource = 0; resource < 5; resource++) {
			printf("%d\t",
			       Data->resourcesPool[simulation][resource]);
		}
		printf("\t");
		for (act = 0; act < NUM_ACTIONS; act++) {
			printf("%d\t",
			       Data->conditionsMet[simulation][act]);
		}
		printf("\n");
	}
	printf
	    ("BRICK\tLUMBER\tGRAIN\tWOOL\tORE\t\tSET\tCIT\tDEV\tRSET\tRRSET\tS+SET\tS+CIT\tS+DEV\tS+RSET\tS+RRSET\tC+CIT\tC+DEV\tC+RSET\tC+RRSET\tD+DEV\tD+RSET\tD+RRSET\tR+RSET\tR+RRSET\tRR+RRSET\n");
	printf("\nNumber of Simulations OK for every action->");
	for (act = 0; act < NUM_ACTIONS; act++) {
		printf("\t%d", Data->numberOfSimulationsOK[act]);
	}
	printf("\nTurns to Action for every action->\t");
	for (act = 0; act < NUM_ACTIONS; act++) {
		printf("\t%d", Data->turnsToAction[act]);
	}
	printf("\n");
	printf("Turn %d\n", turn);
}

void set_timeCombinedAction(struct simulationsData_t *Data)
{
	/*      Puts the information of turnsToAction in a way that is easier to access from bestStrategy procedure
	 *                  SET     CIT     DEV     RSET    RRSET
	 *          SET     5       6       7       8       9   ------->for example, means turnsToAction[9];
	 *          CIT     6       10      11      12      13
	 *          DEV     7       11      14      15      16
	 *          RSET    8       12      15      17      18
	 *          RRSET   9       13      16      18      19  */

	Data->timeCombinedAction[0][0] = Data->turnsToAction[5];
	Data->timeCombinedAction[0][1] = Data->turnsToAction[6];
	Data->timeCombinedAction[0][2] = Data->turnsToAction[7];
	Data->timeCombinedAction[0][3] = Data->turnsToAction[8];
	Data->timeCombinedAction[0][4] = Data->turnsToAction[9];
	Data->timeCombinedAction[1][0] = Data->turnsToAction[6];
	Data->timeCombinedAction[1][1] = Data->turnsToAction[10];
	Data->timeCombinedAction[1][2] = Data->turnsToAction[11];
	Data->timeCombinedAction[1][3] = Data->turnsToAction[12];
	Data->timeCombinedAction[1][4] = Data->turnsToAction[13];
	Data->timeCombinedAction[2][0] = Data->turnsToAction[7];
	Data->timeCombinedAction[2][1] = Data->turnsToAction[11];
	Data->timeCombinedAction[2][2] = Data->turnsToAction[14];
	Data->timeCombinedAction[2][3] = Data->turnsToAction[15];
	Data->timeCombinedAction[2][4] = Data->turnsToAction[16];
	Data->timeCombinedAction[3][0] = Data->turnsToAction[8];
	Data->timeCombinedAction[3][1] = Data->turnsToAction[12];
	Data->timeCombinedAction[3][2] = Data->turnsToAction[15];
	Data->timeCombinedAction[3][3] = Data->turnsToAction[17];
	Data->timeCombinedAction[3][4] = Data->turnsToAction[18];
	Data->timeCombinedAction[4][0] = Data->turnsToAction[9];
	Data->timeCombinedAction[4][1] = Data->turnsToAction[13];
	Data->timeCombinedAction[4][2] = Data->turnsToAction[16];
	Data->timeCombinedAction[4][3] = Data->turnsToAction[18];
	Data->timeCombinedAction[4][4] = Data->turnsToAction[19];
}

void numberOfTurnsForProbability(float probability,
				 struct simulationsData_t *Data,
				 struct gameState_t myGameState,
				 int showSimulation)
{
	/* Sets turnsToAction values to the number of turns needed to have a certain probability to get the resources needed to perform each NUM_ACTIONS possible actions
	 * It does so by simulating MAX_SIMS times the dice outcomes of a single turn and checking how many of those simulations would fullfill the requirements of
	 * resourcesNeededForAction of every action, and updating numberOfSimulationsOK and conditionsMet matrix consequently
	 * When the percentage of simulations that meet the requirements for a certain action is over probability, then it means that given that amount of turns,
	 * then that percentage of simulations would fullfill those requirements, and it will set that number of turns for that action in turnsToAction.
	 * At the end of the process turnsToAction will hold the number of turns needed for every possible action to be performed with the required probability.
	 * The number of simulations MAX_SIMS could be easily increased in order to get a more accurate simulation process*/

	int currentTurn = 0;
	int simulation;
	int dice_roll1, dice_roll2, dice_roll;
	int i, j;

	/*Initialize Data->resourcesPool to resourcesAlreadyHave[] for every resource and conditionsMet to FALSE for every action */
	for (i = 0; i < MAX_SIMS; i++) {
		for (j = 0; j < 5; j++) {
			Data->resourcesPool[i][j] =
			    myGameState.resourcesAlreadyHave[j];
		}
		for (j = 0; j < NUM_ACTIONS; j++) {
			Data->conditionsMet[i][j] = 0;
		}
	}
	/*Init turnsToAction to the maximum and numberOfSimulationsOK to 0  */
	for (i = 0; i < NUM_ACTIONS; i++) {
		Data->turnsToAction[i] = MAX_TURNS;
		Data->numberOfSimulationsOK[i] = 0;
	}
	updateConditionsMet(probability, Data);	/*Some conditions could already be met at the beginning */
	updateTurnsToAction(probability, currentTurn, Data);
	if (showSimulation) {
		outputSims(30, currentTurn, Data);
		printf("Press any key to run the simulation");
		getchar();
	}
	srandom(time(NULL));
	while (currentTurn < MAX_TURNS) {	/*It will simulate up to MAX_TURNS-1 for simplicity sake. */
		/*Should it be optimized? What is the chance of being able to perform everything in a number of turns before the maximum? Maybe with a very low level of probability and under
		 *certain circumstances, but very uncommon in any case.*/
		currentTurn++;
		for (simulation = 0; simulation < MAX_SIMS; simulation++) {
			/* For every simulation it rolls the dice and increases its resources accordingly
			 * CHECK THIS PART, THIS IS NOT A VERY GOOD RANDOM DICE ROLL*/
			dice_roll1 = (random() % 6) + 1;
			dice_roll2 = (random() % 6) + 1;
			dice_roll = dice_roll1 + dice_roll2;
			/*printf("\n(%d+%d)=%d\n ",dice_roll1,dice_roll2,dice_roll); */
			/*updates resourcesPool for this simulation */
			for (i = 0; i <= 4; i++) {
				Data->resourcesPool[simulation][i] =
				    Data->resourcesPool[simulation][i] +
				    myGameState.resourcesSupply[dice_roll -
								2][i];
				/*printResource(i);printf("+%d=",resourcesSupply[dice_roll-2][i]);printf("%d ",Data->resourcesPool[simulation][i]); */
			}
		}
		/*End of all the simulations for this turn, all simulations have their resourcesPool resources updated according to their dice rolls.
		 *Now check for every simulation if conditions are met and update it, and numberOfSimulationsOK accordingly*/
		updateConditionsMet(probability, Data);
		/*Check for every action if it is OK enough times and update turnsToAction for that action to turn if needed */
		updateTurnsToAction(probability, currentTurn, Data);
		if (showSimulation) {
			outputSims(30, currentTurn, Data);
			/*printf("Press any key to next turn");
			   getchar(); */
		}
	}			/*while */
	set_timeCombinedAction(Data);
	return;
}

float strategyProfit(float time_a, float time_b, float turn,
		     strategy_t oneStrategy,
		     struct gameState_t myGameState)
{
	/*Returns the benefit I get in a given turn if I try to do firstAction ending at time_a and then secondAction ending at time_b
	 * time_a, time_b and turn should be possitive numbers, turn>=0, time_b>=time_a>=0*/
	float firstActionValue = myGameState.actionValue[oneStrategy[0]];
	float secondActionValue = myGameState.actionValue[oneStrategy[1]];
	float m1, m2;
	if (turn < time_a) {	/*so time_a is not 0 */
		if (time_a == time_b) {
			return (((firstActionValue +
				  secondActionValue) / time_a) * turn);
		} else {
			m1 = (firstActionValue / time_a);
			return (m1 * turn);
		}
	} else if (turn < time_b) {	/*So time_b is not 0, although time_a could be */
		m2 = (secondActionValue / (time_b - time_a));
		return (firstActionValue + m2 * (turn - time_a));
	} else if (turn >= time_b) {
		return (firstActionValue + secondActionValue);
	} else
		return (0);	/*It should never get here */

	/*Notice how extreme cases are handled:
	   If a=b=0 it means I can do both actions at the same time right now and it will go through the third branch no matter the turn.
	   If a=b but not 0 it means I can do both actions at the same time in the future, and it will go through the first branch if turn is before that moment
	   and through the third from that moment on.
	   If a=0 but not b, it means I can do my first action right now, it will go through the second branch until b is reached. */
}

float bestStrategy(float turn, float probability,
		   struct simulationsData_t *Data, strategy_t myStrategy,
		   struct gameState_t myGameState, int showSimulation)
{
	/*Updates myStrategy[] with the pair of preferred actions to be performed in the present/future as their profit is considered when we look up to turn in the future
	 * It returns that maximum profit*/
	float time_a, time_b, time_best_firstAction,
	    time_best_secondAction;
	enum action firstAction, secondAction;
	float profit, max_profit;
	strategy_t oneStrategy;

	numberOfTurnsForProbability(probability, Data, myGameState, showSimulation);	/*Sets Data->turnsToAction by simulating */
	time_best_firstAction = MAX_TURNS;
	time_best_secondAction = MAX_TURNS;
	max_profit = 0;
	for (firstAction = SET; firstAction <= 4; firstAction++) {	/*First five actions of turnsToAction are individual actions SET, CIT, DEV, RSET and RRSET */
		time_a = Data->turnsToAction[firstAction];
		for (secondAction = SET; secondAction <= 4; secondAction++) {
			time_b = Data->timeCombinedAction[firstAction]
			    [secondAction];
			oneStrategy[0] = firstAction;
			oneStrategy[1] = secondAction;
			profit =
			    strategyProfit(time_a, time_b, turn,
					   oneStrategy, myGameState);
			if ((profit > max_profit)
			    || ((profit == max_profit)
				&& (time_a < time_best_firstAction))
			    || ((profit == max_profit)
				&& (time_a == time_best_firstAction)
				&& (time_b < time_best_secondAction))) {
				/*Updates the best strategy if:
				 * 1 - The profit calculated iś better than the profit calculated so far OR
				 * 2 - Being equal, this strategy lets me do the firstAction before OR
				 * 3 - Being equal the profit and the time needed to perform firstAction, this strategy lets me do secondAction before*/
				max_profit = profit;
				myStrategy[0] = firstAction;
				time_best_firstAction = time_a;
				myStrategy[1] = secondAction;
				time_best_secondAction = time_b;
			}
		}
	}
	return (max_profit);
}

void updateTradingMatrix(float turn, float prob, float profit,
			 struct tradingMatrixes_t *tradeThisForThat,
			 struct gameState_t myGameState)
{
	/*It will check if any resource trade will improve my maxprofit in a given turn and update the trading matrix consequently.
	 * At this point it will only check whether any one resource by one resource trading is beneficial to me, and will not consider wich of those beneficial
	 * tradings is the most beneficial in case there was more than one.
	 * If a 4:1 trading is beneficial it will set tradeThisForThat to 4, if it is a 1:1 it will set to 1
	 * IMPROVEMENT: Use a constant k[1..2] so that I am only interested in trading if profitAfterTrade is k times higher than profit.
	 * That k could improve when Im trading with "winning" adversaries.*/
	int give, take;		/*I give resource give, I get resource take */
	float profitAfterTrade;
	struct simulationsData_t thisSimulation;
	strategy_t thisStrategy;

	for (give = 0; give <= 4; give++) {
		for (take = 0; take <= 4; take++) {
			tradeThisForThat->bankTrade[give][take] = 0;
			tradeThisForThat->internalTrade[give][take] = 0;
		}
	}
	for (give = 0; give <= 4; give++) {
		if (myGameState.resourcesAlreadyHave[give] >= 4) {	/*I have at least 4 of this resource, I could do 4:1 trade */
			for (take = 0; take <= 4; take++) {
				if (give != take) {	/*There is no point in trading the same resource */
					myGameState.resourcesAlreadyHave
					    [give] =
					    myGameState.
					    resourcesAlreadyHave[give] - 4;
					myGameState.
					    resourcesAlreadyHave[take]++;
					profitAfterTrade =
					    bestStrategy(turn, prob,
							 &thisSimulation,
							 thisStrategy,
							 myGameState, 0);
					if (profitAfterTrade > profit) {
						tradeThisForThat->
						    bankTrade[give]
						    [take] = 1;
						printf
						    ("\t\tI could be interested in trading 4 ");
						printResource(give);
						printf
						    (" in exchange of 1 ");
						printResource(take);
						printf
						    (", because that would raise my expected profit to %f ",
						     profitAfterTrade);
						printf("[By doing ");
						printAction(thisStrategy
							    [0]);
						printf(" at time %d",
						       thisSimulation.turnsToAction
						       [thisStrategy[0]]);
						printf(" and ");
						printAction(thisStrategy
							    [1]);
						printf(" at time %d]\n",
						       thisSimulation.timeCombinedAction
						       [thisStrategy[0]]
						       [thisStrategy[1]]);
					}
					myGameState.resourcesAlreadyHave
					    [give] =
					    myGameState.
					    resourcesAlreadyHave[give] + 4;
					myGameState.
					    resourcesAlreadyHave[take]--;
				}
			}
		}
		if (myGameState.resourcesAlreadyHave[give]) {	/*I cannot trade something I dont have */
			for (take = 0; take <= 4; take++) {
				if (give != take) {	/*There is no point in trading the same resource */
					myGameState.resourcesAlreadyHave
					    [give]--;
					myGameState.resourcesAlreadyHave
					    [take]++;
					profitAfterTrade =
					    bestStrategy(turn, prob,
							 &thisSimulation,
							 thisStrategy,
							 myGameState, 0);
					if (profitAfterTrade > profit) {
						tradeThisForThat->
						    internalTrade[give]
						    [take] = 1;
						printf
						    ("\t\tIm interested in giving ");
						printResource(give);
						printf(" in exchange of ");
						printResource(take);
						printf
						    (", because that would raise my expected profit to %f ",
						     profitAfterTrade);
						printf("[By doing ");
						printAction(thisStrategy
							    [0]);
						printf(" at time %d",
						       thisSimulation.turnsToAction
						       [thisStrategy[0]]);
						printf(" and ");
						printAction(thisStrategy
							    [1]);
						printf(" at time %d]\n",
						       thisSimulation.timeCombinedAction
						       [thisStrategy[0]]
						       [thisStrategy[1]]);
					}
					myGameState.resourcesAlreadyHave
					    [give]++;
					myGameState.resourcesAlreadyHave
					    [take]--;
				}
			}
		}
	}
}

int checkRoadNow(enum action oneAction, struct gameState_t myGameState)
{
	/*Returns TRUE if action involves building a Road and it is possible to build it now */
	switch (oneAction) {
	case RSET:
		if ((myGameState.resourcesAlreadyHave[0] >= 1)
		    && (myGameState.resourcesAlreadyHave[1] >= 1))
			return (1);
		else
			return (0);
		break;
	case RRSET:
		if ((myGameState.resourcesAlreadyHave[0] >= 2)
		    && (myGameState.resourcesAlreadyHave[1] >= 2))
			return (2);
		else if ((myGameState.resourcesAlreadyHave[0] >= 1)
			 && (myGameState.resourcesAlreadyHave[1] >= 1))
			return (1);
		else
			return (0);
		break;
	default:
		return (0);
	}
}




int main(int argc, char **argv)
{
	float max_profit_returned;
	float time_a, time_b, prob, turn;
	int roadsNow, verbose;
	char option;
	struct simulationsData_t oneSimulation;
	struct gameState_t oneState;
	strategy_t oneStrategy;
	struct tradingMatrixes_t tradeThisForThat;
	oneState = (struct gameState_t) {	/*Depends on my location on the map */
		{
			{0, 0, 0, 0, 1},	/*Rolls 2 */
			{1, 0, 0, 0, 0},	/*Rolls 3 */
			{1, 1, 0, 0, 0},	/*Rolls 4 */
			{0, 0, 0, 0, 0},	/*Rolls 5 */
			{0, 0, 0, 1, 0},	/*Rolls 6 */
			{0, 0, 0, 0, 0},	/*Rolls 7, always 0 resources */
			{0, 0, 3, 0, 0},	/*Rolls 8 */
			{0, 0, 0, 0, 1},	/*Rolls 9 */
			{0, 0, 0, 2, 0},	/*Rolls 10 */
			{0, 0, 0, 0, 0},	/*Rolls 11 */
			{0, 0, 1, 0, 0},	/*Rolls 12 */
		},		/*Represents the eleven different dice outcomes (2-12) crossed with the amount of resources they produce (of Brick, Lumber, Grain, Wool and Ore) */
		{3, 3, 1, 0, 4},	/*Resources I already have of Brick, Lumber, Grain, Wool and Ore */
		{10, 14, 5, 12, 14}	/*Benefit of doing best SET, best CIT, pick DEV, best RSET and best RRSET */
	};
	/*prob and turn are values that are fixed for every Genetic Player (and evolved by the Genetic Algorithm). For a certain player those values will be fixed for the whole game.
	 * Other values that will be set for a particular player will be a resourcesValueMatrix, that will give a certain weight to every kind of resource
	 * at every point in the game, and a depreciation constant k, that will decrease the value given to resources I have as opposite to resources I dont have.
	 * Those resourcesValueMatrix and constant k, in combination with the game situation, will basically influence how actionValue is set at a particular point in the game
	 * At this point of the program actionValue is set arbitrarily to the values I chose*/
	system("clear");
	prob = 0.5001;		/*in case is not set */
	verbose = 0;		/*if it is set to 1 it will output the simulations */

	printf
	    ("This piece of code outputs the preferred action to be taken in a given turn according to the values passed to it.\n");
	printf
	    ("Resource Values Matrix and depreciation constant k are ignored in this test for simplicity sake, as they only affect how actionValue values are calculated.\n");
	printf
	    ("Enter actionValue values for Settlement, City, Development, Road to Settlement and Long Road to Settlement separated by blank spaces:\n");
	scanf("%f %f %f %f %f", &(oneState.actionValue[0]),
	      &(oneState.actionValue[1]), &(oneState.actionValue[2]),
	      &(oneState.actionValue[3]), &(oneState.actionValue[4]));
	printf
	    ("Enter actual resources pool of Wood, Lumber, Grain, Wool and Ore separated by blank spaces (integers):\n");
	scanf("%d %d %d %d %d", &(oneState.resourcesAlreadyHave[0]),
	      &(oneState.resourcesAlreadyHave[1]),
	      &(oneState.resourcesAlreadyHave[2]),
	      &(oneState.resourcesAlreadyHave[3]),
	      &(oneState.resourcesAlreadyHave[4]));
	printf
	    ("Enter value for probability (this represents the level of certainty the AI requires to assert it will get the resources needed) (0..1):\n");
	scanf("%f", &prob);
	printf("Do you want to see how the simulation is working?(s/n)");
	while (((option = getchar()) != EOF) & (option != '\n'))	/* This will eat up all other char other characters*/
		;
	option = getchar();
	switch (option) {
	case 'S':
	case 's':
		verbose = 1;
		break;
	default:
		verbose = 0;
	}
	printf("Calculations made using a probability of %f\n", prob);
	printf
	    ("The best strategy according to the profit calculated when I look up to certain turn is to do:\n\n");
	for (turn = 4; turn < 20; turn++) {	/*In a real game it will use a fixed value for turn */
		max_profit_returned =
		    bestStrategy(turn, prob, &oneSimulation, oneStrategy,
				 oneState, verbose);
		time_a = oneSimulation.turnsToAction[oneStrategy[0]];
		time_b = oneSimulation.timeCombinedAction[oneStrategy[0]]
		    [oneStrategy[1]];
		printf("Turn %.2f >>\t", turn);
		printAction(oneStrategy[0]);
		printf(" at time %.2f (%.2f pts.) and then ", time_a,
		       oneState.actionValue[oneStrategy[0]]);
		printAction(oneStrategy[1]);
		printf
		    (" at time %.2f (%.2f pts.) [Expected profit at turn %.2f=%.2f]\n",
		     time_b, oneState.actionValue[oneStrategy[1]], turn,
		     max_profit_returned);
		roadsNow = checkRoadNow(oneStrategy[0], oneState);
		switch (roadsNow) {
		case 1:
			printf("\t\tDoing Road right now!\n");
			break;
		case 2:
			printf("\t\tDoing 2 Roads right now!\n");
			break;
		}
		updateTradingMatrix(turn, prob, max_profit_returned,
				    &tradeThisForThat, oneState);
		if (verbose) {
			printf("Press any key to make another simulation");
			getchar();
		}
		printf("\n");
	}
	return 0;
}
