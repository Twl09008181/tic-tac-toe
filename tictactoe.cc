#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <limits>
#include <math.h>
#include <iomanip>


struct node{
  std::string state; //O--X-----
  std::vector<std::unique_ptr<node>>children;
  float val;
  int vist;
  float uct;
  char player;
  node* parent;
  node(std::string s, node*p=nullptr, char play='O')
    :
    state{s},
    parent{p},
    player{play},
    val{0},
    vist{0},
    uct{std::numeric_limits<float>::max()}
  {}
  void addChild(std::string nextState, char nextPlayer){
    children.push_back(std::make_unique<node>(nextState, this, nextPlayer));
  }

  node* getLargest();
  bool isLeaf()const{return children.size()==0;}
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


class mcts{

public:
  mcts()
    :top{std::make_unique<node>("---------", nullptr, 'X')}
  {
  }
  void run(int iterations=1000)
  {
    int it = 1;
    while(it < iterations){
      node* cur = selection(it);
      cur = expansion(cur);
      char result = rollout(cur);
      backpropagation(cur, result, it);
      it++;
    }
  }
  node* getTop(){return top.get();}
private:
  void updateUCT(node*cur, int iter);
  std::unique_ptr<node>top;
  node* selection(int);
  node* expansion(node*cur);
  char rollout(node*cur);// 'D':draw, 'O','X'
  void backpropagation(node*cur, char result, int totalIter);
};

void mcts::updateUCT(node*cur, int iter){
  if(cur->vist!=0){
    cur->uct = (cur->val / cur->vist) + 2 * sqrt(log(float(iter))/(cur->vist));
  }
}

node* node::getLargest(){
  node*best = children.front().get();
  for(auto &child:children){
    if(child->val > best->val){
      best = child.get();
    }
  }
  return best;
}

auto mcts::selection(int iter) -> node*
{
  node* cur = top.get();
  while(!cur->isLeaf()){ 
    float maxuct = 0;
    int bestChildId = 0;
    for(int id = 0; id < cur->children.size(); ++id){
      updateUCT(cur->children[id].get(),iter);
      if(cur->children[id]->uct > maxuct){
        maxuct = cur->children[id]->uct;
        bestChildId = id;
      }
    }
    cur = cur->children[bestChildId].get();
  }
  return cur;
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

auto mcts::expansion(node*cur) -> node*
{

  //do expansion
  if(cur->vist != 0 || cur == top.get()){
    char nextPlayer;
    if(cur->player=='O')
      nextPlayer = 'X'; 
    else
      nextPlayer = 'O'; 

    std::vector<int>emptyPosition = findEmptyPos(cur->state);
    if(emptyPosition.size()==0)
      return cur;

    for(int i = 0; i < emptyPosition.size(); i++){
      int pos = emptyPosition[i];
      std::string nextState = cur->state;
      nextState[pos] = nextPlayer;//update
      cur->addChild(nextState, nextPlayer);
    }

    cur = cur->children[0].get();
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



int randomPick(std::vector<int>&emptyPos)
{
  return emptyPos[rand() % emptyPos.size()];
}

char mcts::rollout(node*cur)
{
  std::string state = cur->state;
  char playerNow = cur->player;

  bool O_isWin = isWin('O', state);
  bool X_isWin = isWin('X', state);
  std::vector<int>emptyPosition = findEmptyPos(cur->state);

  int i = 0;
  //not terminate
  while(!O_isWin && !X_isWin && emptyPosition.size()!=0)
  {
    //update state
    char player;
    if(playerNow == 'O')
      player = 'X';
    else
      player = 'O';

    //find an empry posistion randomly.
    state[randomPick(emptyPosition)] = player;
    playerNow = player;

    O_isWin = isWin('O', state);
    X_isWin = isWin('X', state);
    emptyPosition = findEmptyPos(state);
  }

  if(O_isWin)
    return 'O';
  else if(X_isWin)
    return 'X';
  else
    return 'D';
}


void mcts::backpropagation(node*cur, char result, int totalIter)
{ 
  while(cur != top.get()){

    if(result != 'D'){
      if(result == cur->player){
        cur->val+=2;
      }
      else{
        cur->val-=2;
      }
    }
    else{
      if(cur->player=='X')
        cur->val+=1;
    }
    cur->vist++;
    cur = cur -> parent;
  }
}

//find child which state ==  nextState
node* findChild(node* currentNode, std::string nextState){
  for(auto&child:currentNode->children){
    if(child->state == nextState)
      return child.get();
  }
  return nullptr;
}




node* machinePlay(node* current){
  if(!current->isLeaf())
    return current->getLargest();
  else{
    std::cout<<"simulation too small\n";
    exit(1);
    return current;
  }
}


int main(int argc, char**argv){
  
  mcts myMcts;
  //myMcts.run(std::stoi(argv[1]));
  myMcts.run(1000000);

  bool endGame = false;
  while(!endGame){
    std::cout<<"start a new game\n";
    node*n = myMcts.getTop();
    std::string state = n->state;
    bool O_isWin = isWin('O', state);
    bool X_isWin = isWin('X', state);
    std::vector<int>emptyPosition = findEmptyPos(state);

    while(!O_isWin && !X_isWin && emptyPosition.size()!=0)
    {
      bool readInput = false;
      while(!readInput){
        int x,y;
        std::cout<<"enter position\n";
        std::cin >> x >> y;
        if(x<0 || x>2 || y<0 ||y>2)
          std::cout<<"wrong position\n";
        else if(state[y*3+x] != '-'){
          std::cout<<"non empty pos\n";
        }
        else{
          std::cout<<"Human stage\n";
          readInput = true;
          state[y*3+x] = 'O';
          show(state);
          n = findChild(n, state);
          if(n == nullptr){ 
            std::cout<<"simulation too small\n";
            exit(1);
          }
        }
      }
      O_isWin = isWin('O', state);
      X_isWin = isWin('X', state);
      emptyPosition = findEmptyPos(state);
      if(!O_isWin && !X_isWin && emptyPosition.size()!=0){
        std::cout<<"Machine stage\n";
        n = machinePlay(n);
        state = n->state;
        show(state);
        O_isWin = isWin('O', state);
        X_isWin = isWin('X', state);
        emptyPosition = findEmptyPos(state);
      }
    }
    if(O_isWin){
      std::cout<<"Human win!\n";
    }
    else if(X_isWin){
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