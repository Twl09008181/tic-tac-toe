#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <limits>
#include <math.h>
#include <iomanip>
#include <cassert>


struct node{
  std::string state; //O--X-----
  std::vector<std::unique_ptr<node>>children;
  double val;
  int visit;
  char player;
  node* parent;
  node(std::string s, node*p=nullptr, char play='O')
    :
    state{s},
    parent{p},
    player{play},
    val{0},
    visit{0}
  {}
  void addChild(std::string nextState, char nextPlayer){
    children.push_back(std::make_unique<node>(nextState, this, nextPlayer));
  }
  bool isLeaf()const{
    return children.size() == 0;
  }
};



void show(std::string state){
  for(int i = 0; i < 9; i++){
    if(i%3==0){
      std::cout<<"\n";
    }
    std::cout<<state[i]<<" ";
  }
  std::cout<<"\n";
}


// uct = vi / ni + C * sqrt(ln(Ni) / ni), Ni = #visits of parent
double caculateUCT(node* cur)
{
  double C = 1.416;
  if(cur->visit == 0)
    return std::numeric_limits<double>::max();
  double uct = (cur->val / cur->visit);
  if(cur->parent->visit != 0){
    uct += C * sqrt(log(cur->parent->visit)/(cur->visit));
  }
  return uct;
}

// return the child with largest uct value
node* getBestUctChild(node* cur){
  node* bestChild = nullptr;
  double maxUCT = std::numeric_limits<double>::lowest();
  for(auto& child:cur->children){
    double uct = caculateUCT(child.get());
    if(uct > maxUCT){
      maxUCT =  uct;
      bestChild = child.get();
    }
  }
  return bestChild;
}

class mcts{

public:
  mcts(std::string state, char userPlayer='O')
    :top{std::make_unique<node>(state, nullptr, userPlayer)}
  {
  }
  node* run(int iterations=1000)
  {
    int it = 1;
    while(it < iterations){
      node* cur = selection();
      cur = expansion(cur);
      char result = rollout(cur);
      backpropagation(cur, result);
      it++;
    }
//    for(auto &child:top->children){
//      show(child->state);
//      std::cout<<"val:"<<child->val<<"  ";
//      std::cout<<"vst:"<<child->visit<<"  ";
//      std::cout<<"uct:"<<child->uct<<"\n";
//    }

    node* child = getBestUctChild(top.get());
    assert(child != nullptr && "state is already end");
    return child;
  }
  node* getTop(){return top.get();}
private:
  std::unique_ptr<node>top;
  node* selection();
  node* expansion(node*cur);
  char rollout(node*cur);// 'D':draw, 'O','X'
  void backpropagation(node*cur, char result);
};


// selection best uct from top.
auto mcts::selection() -> node*
{
  node* cur = top.get();
  while(!cur->isLeaf()){ 
    cur = getBestUctChild(cur);
  }
  return cur;
}

bool isWin(char player, std::string state){
  for(int row = 0; row < 9; row+=3){
    if(state[row] == player &&
        state[row+1] == player &&
        state[row+2] == player)
      return true;
  }
  for(int col = 0; col < 3; col++){
    if(state[col] == player &&
        state[col+3] == player &&
        state[col+6] == player)
      return true;
  }
  if(state[0] == player && state[4]==player && state[8] == player)
    return true;
  if(state[2] == player && state[4]==player && state[6] == player)
    return true;
  return false;
}


std::vector<int> findEmptyPos(std::string state)
{
  std::vector<int>emptyPosition;
  for(int i = 0; i < 9; ++i){
    if(state[i] == '-')
      emptyPosition.push_back(i);
  }
  return emptyPosition;
}

struct gameState{ 
  bool Owin;
  bool Xwin;
  std::vector<int>emptyPosition;
  gameState(std::string state)
    :
      Owin{isWin('O', state)},
      Xwin{isWin('X', state)},
      emptyPosition{findEmptyPos(state)}
  {

  }
  bool isEnd(){
    return Owin || Xwin || emptyPosition.size()==0;
  }
  void refresh(std::string state){
    Owin = isWin('O', state);
    Xwin = isWin('X', state);
    emptyPosition = findEmptyPos(state);

  }
};

// return itself if visit = 0, otherwise do expansion and return it's child
auto mcts::expansion(node*cur) -> node*
{

  gameState game(cur->state);

  //do expansion
  if(!game.isEnd() && (cur->visit != 0 || cur == top.get())){

    assert(cur->children.size()==0 && "already expanded");

    char nextPlayer = (cur->player == 'O') ? 'X' : 'O';

    for(int i = 0; i < game.emptyPosition.size(); i++){
      int pos = game.emptyPosition[i];
      std::string nextState = cur->state;
      nextState[pos] = nextPlayer; //update board state
      cur->addChild(nextState, nextPlayer);
    }
    cur = cur->children[0].get();
  }
  return cur;
}


int randomPick(std::vector<int>&emptyPos)
{
  return emptyPos[rand() % emptyPos.size()];
}

// return the game result from this current node by ramdom actions.
char mcts::rollout(node*cur)
{
  std::string state = cur->state;
  char lastPlayer = cur->player;

  gameState game(state);

  //not terminate
  while(!game.isEnd())
  {
    // update state
    char player = (lastPlayer == 'O') ? 'X' : 'O';
    // find an empry posistion randomly.
    state[randomPick(game.emptyPosition)] = player;
    // update player
    lastPlayer = player;
    // update game state
    game.refresh(state);
  }

  if(game.Owin)
    return 'O';
  else if(game.Xwin)
    return 'X';
  else
    return 'D';
}


// update node->visit and val according to game result
void mcts::backpropagation(node*cur, char result)
{ 
  while(cur != nullptr){ 

    if(result != 'D'){
      if(result == cur->player){
        cur->val++;
      }
      else{
        cur->val--;
      }
    }
    cur->visit++;
    cur = cur -> parent;
  }
}


std::string userPlay(std::string state, char player){
  bool readInput = false;
  while(!readInput){
    int x;
    std::cout<<"enter position 1~9\n";
    std::cin >> x ;
    if(x<1 || x>9)
      std::cout<<"wrong position\n";
    else if(state[x-1] != '-'){
      std::cout<<"non empty pos\n";
    }
    else{
      std::cout<<"Human stage\n";
      readInput = true;
      state[x-1] = player;
    }
  }
  return state;
}


int main(int argc, char**argv){
  
  int iteration = 5000;

  if(argc==2){
    iteration = std::stoi(argv[1]);
  }

  while(1){
    std::cout<<"start a new game\n";
    std::string state = "---------";
    gameState game(state);
    srand(time(0));

    std::cout<<"choose player O or X\n";
    char userPlayer;
    std::cin >> userPlayer;

    if(userPlayer == 'O'){
      while(!game.isEnd()){
        state = userPlay(state, userPlayer);
        show(state);
        game.refresh(state);
        if(!game.isEnd()){
          state = mcts(state, userPlayer).run(iteration)->state;
          show(state);
          game.refresh(state);
        }
      }
    }
    else{
      while(!game.isEnd()){
        state = mcts(state, userPlayer).run(iteration)->state;
        show(state);
        game.refresh(state);
        if(!game.isEnd()){
          state = userPlay(state, userPlayer);
          show(state);
          game.refresh(state);
        }
      }
    }

    if(game.Owin){ 
      if(userPlayer == 'O')
        std::cout<<"Human win!\n";
      else
        std::cout<<"AI win!\n";
    }
    else if(game.Xwin){
      if(userPlayer == 'X')
        std::cout<<"Human win!\n";
      else
        std::cout<<"AI win!\n";
    }
    else{
      std::cout<<"Draw!\n";
    }
    std::cout<<"enter anything to start new game, press x to exit game!\n";
    char op;
    std::cin>>op;
    if(op=='x')
      break;
  }
  return 0;
}
