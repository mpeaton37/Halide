#include "IRNode.h"
#include <stdarg.h>

void panic(const char *fmt, ...) {
    char message[1024];
    va_list arglist;
    va_start(arglist, fmt);
    vsnprintf(message, 1024, fmt, arglist);
    va_end(arglist);
    printf(message);
    exit(-1);
}

void assert(bool cond, const char *fmt, ...) {
    if (cond) return;
    char message[1024];
    va_list arglist;
    va_start(arglist, fmt);
    vsnprintf(message, 1024, fmt, arglist);
    va_end(arglist);
    printf(message);
    exit(-1);
}


map<float, IRNode *> IRNode::floatInstances;
map<int, IRNode *> IRNode::intInstances;
map<OpCode, IRNode *> IRNode::varInstances;
vector<IRNode *> IRNode::allNodes;



IRNode *IRNode::make(float v) {
    if (floatInstances[v] == NULL) 
        return (floatInstances[v] = new IRNode(v));
    return floatInstances[v];
};

IRNode *IRNode::make(int v) {
    if (intInstances[v] == NULL) 
        return (intInstances[v] = new IRNode(v));
    return intInstances[v];
};

IRNode *IRNode::make(OpCode opcode, 
                     IRNode *input1,
                     IRNode *input2,
                     IRNode *input3,
                     IRNode *input4,
                     int ival,
                     float fval) {
    
    // collect the inputs into a vector
    vector<IRNode *> inputs;
    if (input1) {
        inputs.push_back(input1);
    }
    if (input2) {
        inputs.push_back(input2);
    }
    if (input3) {
        inputs.push_back(input3);
    }
    if (input4) {
        inputs.push_back(input4);
    }
    return make(opcode, inputs, ival, fval);
}

IRNode *IRNode::make(OpCode opcode,
                     vector<IRNode *> inputs,
                     int ival, float fval) {
    
    // We will progressively modify the inputs to finally return a
    // node that is equivalent to the requested node on the
    // requested inputs. 
    
    // First, type inference and coercion
    Type t;
    switch(opcode) {
    case Const:
        panic("Shouldn't make Consts using this make function\n");
    case NoOp:
        assert(inputs.size() == 1, "Wrong number of inputs for opcode: %s %d\n",
               opname[opcode], inputs.size());
        t = inputs[0]->type;
        break;
    case VarX: 
    case VarY: 
    case VarT:
    case VarC:
    case UnboundVar:
        assert(inputs.size() == 0, "Wrong number of inputs for opcode: %s %d\n",
               opname[opcode], inputs.size());
        t = Int;
        break;
    case Plus:
    case Minus:
    case Times:
    case Power:
    case Mod:
        assert(inputs.size() == 2, "Wrong number of inputs for opcode: %s %d\n",
               opname[opcode], inputs.size());
        // the output is either int or float
        if (inputs[0]->type == Float ||
            inputs[1]->type == Float) t = Float;
        else t = Int;
        
        // upgrade everything to the output type
        inputs[0] = inputs[0]->as(t);
        inputs[1] = inputs[1]->as(t);
        break;
    case Divide:
    case ATan2:
        t = Float;
        inputs[0] = inputs[0]->as(Float);
        inputs[1] = inputs[1]->as(Float);
        break;
    case Sin:
    case Cos:
    case Tan:
    case ASin:
    case ACos:
    case ATan:
    case Exp:
    case Log:
        assert(inputs.size() == 1, "Wrong number of inputs for opcode: %s %d\n", 
               opname[opcode], inputs.size());
        t = Float;
        inputs[0] = inputs[0]->as(Float);
        break;
    case Abs:
        assert(inputs.size() == 1, "Wrong number of inputs for opcode: %s %d\n", 
               opname[opcode], inputs.size());
        if (inputs[0]->type == Bool) return inputs[0];
        t = inputs[0]->type;
        break;
    case Floor:
    case Ceil:
    case Round:
        assert(inputs.size() == 1, "Wrong number of inputs for opcode: %s %d\n", 
               opname[opcode], inputs.size());    
        if (inputs[0]->type != Float) return inputs[0];
        t = Float; // TODO: perhaps Int?
        break;
    case LT:
    case GT:
    case LTE:
    case GTE:
    case EQ:
    case NEQ:
        assert(inputs.size() == 2, "Wrong number of inputs for opcode: %s %d\n",
               opname[opcode], inputs.size());    
        if (inputs[0]->type == Float || inputs[1]->type == Float) {
            t = Float;                    
        } else { // TODO: compare ints?
            t = Bool;
        }
        inputs[0] = inputs[0]->as(t);
        inputs[1] = inputs[1]->as(t);
        t = Bool;
        break;
    case And:
    case Nand:
        assert(inputs.size() == 2, "Wrong number of inputs for opcode: %s %d\n",
               opname[opcode], inputs.size());    
        // first arg is always bool
        inputs[0] = inputs[0]->as(Bool);
        t = inputs[1]->type;
        break;
    case Or:               
        assert(inputs.size() == 2, "Wrong number of inputs for opcode: %s %d\n",
               opname[opcode], inputs.size());    
        if (inputs[0]->type == Float || inputs[1]->type == Float) {
            t = Float;
        } else if (inputs[0]->type == Int || inputs[0]->type == Int) {
            t = Int;
        } else {
            t = Bool;
        }
        inputs[0] = inputs[0]->as(t);
        inputs[1] = inputs[1]->as(t);
        break;
    case IntToFloat:
        assert(inputs.size() == 1, "Wrong number of inputs for opcode: %s %d\n",
               opname[opcode], inputs.size());
        assert(inputs[0]->type == Int, "IntToFloat can only take integers\n");
        t = Float;
        break;
    case FloatToInt:
        assert(inputs.size() == 1, "Wrong number of inputs for opcode: %s %d\n",
               opname[opcode], inputs.size());
        assert(inputs[0]->type == Float, "FloatToInt can only take floats\n");
        t = Int;
        break;
    case PlusImm:
    case TimesImm:
        assert(inputs.size() == 1,
               "Wrong number of inputs for opcode: %s %d\n",
               opname[opcode], inputs.size());
        t = Int;
        break;
    case LoadImm:
    case Load:
        assert(inputs.size() == 1,
               "Wrong number of inputs for opcode: %s %d\n",
               opname[opcode], inputs.size());
        inputs[0] = inputs[0]->as(Int);
        t = Float;
        break;
        
    }
    
    // Constant folding
    bool allInputsConst = true;
    for (size_t i = 0; i < inputs.size() && allInputsConst; i++) {
        if (inputs[i]->deps) allInputsConst = false;
    }
    if (allInputsConst && inputs.size()) {
        switch(opcode) {

        case Plus:
            if (t == Float) {
                return make(inputs[0]->fval + inputs[1]->fval);
            } else {
                return make(inputs[0]->ival + inputs[1]->ival);
            }
        case Minus:
            if (t == Float) {
                return make(inputs[0]->fval - inputs[1]->fval);
            } else {
                return make(inputs[0]->ival - inputs[1]->ival);
            }
        case Times:
            if (t == Float) {
                return make(inputs[0]->fval * inputs[1]->fval);
            } else {
                return make(inputs[0]->ival * inputs[1]->ival);
            }
        case PlusImm:
            return make(inputs[0]->ival + ival);
        case TimesImm:
            return make(inputs[0]->ival * ival);
        case Divide:
            return make(inputs[0]->fval / inputs[1]->fval);
        case And:
            if (t == Float) {
                return make(inputs[0]->ival ? inputs[1]->fval : 0.0f);
            } else {
                return make(inputs[0]->ival ? inputs[1]->ival : 0);
            }
        case Or:
            if (t == Float) {
                return make(inputs[0]->fval + inputs[1]->fval);
            } else {
                return make(inputs[0]->ival | inputs[1]->ival);
            }
        case Nand:
            if (t == Float) {
                return make(!inputs[0]->ival ? inputs[1]->fval : 0.0f);
            } else {
                return make(!inputs[0]->ival ? inputs[1]->ival : 0);
            }
        case IntToFloat:
            return make((float)inputs[0]->ival);
        case FloatToInt:
            return make((int)inputs[0]->fval);
        default:
            // TODO: transcendentals, pow, floor, comparisons, etc
            break;
        } 
    }

    // Strength reduction rules.
    if (opcode == NoOp) {
        return inputs[0];
    }

    // Replace division of a lower level node with multiplication by its inverse
    if (opcode == Divide && inputs[1]->level < inputs[0]->level) {
        // x/y = x*(1/y)
        return make(Times, inputs[0], 
                    make(Divide, make(1.0f), inputs[1]));
    }

    // (x+a)*b = x*b + a*b (where a and b are both lower level than x)
    if (opcode == Times) {
        IRNode *x = NULL, *a = NULL, *b = NULL;
        if (inputs[0]->op == Plus) {
            b = inputs[1];
            x = inputs[0]->inputs[1];
            a = inputs[0]->inputs[0];
        } else if (inputs[1]->op == Plus) {
            b = inputs[0];
            x = inputs[1]->inputs[1];
            a = inputs[1]->inputs[0];
        }

        if (x) {
            // x is the higher level of x and a
            if (x->level < a->level) {
                IRNode *tmp = x;
                x = a;
                a = tmp;
            }
                    
            // it's only worth rebalancing if a and b are both
            // lower level than x (e.g. (x+y)*3)
            if (x->level > a->level && x->level > b->level) {
                return make(Plus, 
                            make(Times, x, b),
                            make(Times, a, b));
            }
        }

        // deal with const integer a
        if (inputs[0]->op == PlusImm) {
            printf("Hit times of plusimm: ");
            inputs[0]->printExp();
            printf(" * ");
            inputs[1]->printExp();
            printf("\n");
            return make(Plus, 
                        make(Times, inputs[0]->inputs[0], inputs[1]),
                        make(Times, inputs[1], make(inputs[0]->ival)));
        }
    }

    // (x*a)*b = x*(a*b) where a and b are more const than x. This
    // should be replaced with generic product rebalancing,
    // similar to the sum rebalancing.
    if (opcode == Times) {
        IRNode *x = NULL, *a = NULL, *b = NULL;
        if (inputs[0]->op == Times) {
            x = inputs[0]->inputs[0];
            a = inputs[0]->inputs[1];
            b = inputs[1];
        } else if (inputs[1]->op == Times) {
            x = inputs[1]->inputs[0];
            a = inputs[1]->inputs[1];
            b = inputs[0];
        }

        if (x) {
            // x is the higher level of x and a
            if (x->level < a->level) {
                IRNode *tmp = x;
                x = a;
                a = tmp;
            }
                    
            // it's only worth rebalancing if a and b are both
            // lower level than x (e.g. (x+y)*3)
            if (x->level > a->level && x->level > b->level) {
                return make(Times, 
                            x,
                            make(Times, a, b));
            }
        }
    }

    // Some more strength reduction rules that need to be updated for the type system
    /*
      if (opcode == Times) {
      // 0*x = 0
      if (inputs[0]->op == Const &&
      inputs[0]->type == Float &&
      inputs[0]->fval == 0.0f) {
      return make(0.0f);
      } 

      // x*0 = 0
      if (inputs[1]->op == ConstFloat && inputs[1]->val == 0.0f) {
      return make(0);
      }

      // 1*x = x
      if (inputs[0]->op == ConstFloat && inputs[0]->val == 1.0f) {
      return inputs[1];
      } 
      // x*1 = x
      if (inputs[1]->op == ConstFloat && inputs[1]->val == 1.0f) {
      return inputs[0];
      }

      // 2*x = x+x
      if (inputs[0]->op == ConstFloat && inputs[0]->val == 2.0f) {
      return make(Plus, inputTypes[1], inputs[1], inputTypes[1], inputs[1]);
      } 
      // x*2 = x+x
      if (inputs[1]->op == ConstFloat && inputs[1]->val == 2.0f) {
      return make(Plus, inputTypes[0], inputs[0], inputTypes[0], inputs[0]);
      }
      }

      if (opcode == Power && inputs[1]->op == ConstFloat) {
      if (inputs[1]->val == 0.0f) {        // x^0 = 1
      return make(1.0f);
      } else if (inputs[1]->val == 1.0f) { // x^1 = x
      return inputs[0];
      } else if (inputs[1]->val == 2.0f) { // x^2 = x*x
      return make(Times, inputTypes[0], inputs[0], inputTypes[0], inputs[0]);
      } else if (inputs[1]->val == 3.0f) { // x^3 = x*x*x
      IRNode *squared = make(Times, inputTypes[0], inputs[0], inputTypes[0], inputs[0]);
      return make(t, Times, t, squared, inputTypes[0], inputs[0]);
      } else if (inputs[1]->val == 4.0f) { // x^4 = x*x*x*x
      IRNode *squared = make(Times, inputTypes[0], inputs[0], inputTypes[0], inputs[0]);
      return make(Times, t, squared, t, squared);                    
      } else if (inputs[1]->val == floorf(inputs[1]->val) &&
      inputs[1]->val > 0) {
      // iterated multiplication
      int power = (int)floorf(inputs[1]->val);

      // find the largest power of two less than power
      int pow2 = 1;
      while (pow2 < power) pow2 = pow2 << 1;
      pow2 = pow2 >> 1;                                       
      int remainder = power - pow2;
                    
      // make a stack x, x^2, x^4, x^8 ...
      // and multiply the appropriate terms from it
      vector<IRNode *> powStack;
      powStack.push_back(inputs[0]);
      IRNode *result = (power & 1) ? inputs[0] : NULL;
      IRNode *last = inputs[0];
      for (int i = 2; i < power; i *= 2) {
      last = make(Times, last->type, last, last->type, last);
      powStack.push_back(last);
      if (power & i) {
      if (!result) result = last;
      else {
      result = make(Times,
      result->type, result,
      last->type, last);
      }
      }
      }
      return result;
      }

      // todo: negative powers (integer)               
      }            
    */

    // Rebalance summations whenever you hit a node that isn't a sum but might have sums for children.
    if (opcode != Plus && opcode != Minus && opcode != PlusImm) {
        for (size_t i = 0; i < inputs.size(); i++) {
            inputs[i] = inputs[i]->rebalanceSum();
        }
    }              

    // Return a single unique instance of each variable
    if (opcode == VarX || opcode == VarY || opcode == VarT || opcode == VarC) {
        vector<IRNode *> b;
        if (varInstances[opcode] == NULL)
            return (varInstances[opcode] = new IRNode(t, opcode, b, 0, 0.0f));
        return varInstances[opcode];
    }

    // Unbound variables are unique and must not be merged or
    // modified - they will be replaced later individually with a
    // bind() call.
    if (opcode == UnboundVar) {
        vector<IRNode *> b;
        return new IRNode(t, opcode, b, 0, 0.0f);
    }

    // Fuse instructions
        
    // Load of something plus an int constant can be converted to a load with offset
    if (opcode == Load || opcode == LoadImm) {
        if (inputs[0]->op == Plus) {
            IRNode *left = inputs[0]->inputs[0];
            IRNode *right = inputs[0]->inputs[1];
            if (left->op == Const) {
                IRNode *n = make(LoadImm, right, 
                                 NULL, NULL, NULL,
                                 left->ival + ival);
                return n;
            } else if (right->op == Const) {
                IRNode *n = make(LoadImm, left,
                                 NULL, NULL, NULL,
                                 right->ival + ival);
                return n;
            }
        } else if (inputs[0]->op == Minus &&
                   inputs[0]->inputs[1]->op == Const) {
            IRNode *n = make(LoadImm, inputs[0]->inputs[0], 
                             NULL, NULL, NULL, 
                             -inputs[0]->inputs[1]->ival + ival);
            return n;
        } else if (inputs[0]->op == PlusImm) {
            IRNode *n = make(LoadImm, inputs[0]->inputs[0],
                             NULL, NULL, NULL,
                             inputs[0]->ival + ival);
            return n;
        }
    }

    // Times an int constant can be fused into a times immediate
    if (opcode == Times && t == Int) {
        IRNode *left = inputs[0];
        IRNode *right = inputs[1];
        if (left->op == Const) {
            IRNode *n = make(TimesImm, right, 
                             NULL, NULL, NULL,
                             left->ival);
            return n;
        } else if (right->op == Const) {
            IRNode *n = make(TimesImm, left, 
                             NULL, NULL, NULL,
                             right->ival);
            return n;
        }
    }

    // Common subexpression elimination - check if one of the
    // inputs already has a parent that does the same op to the same children.
    if (inputs.size() && inputs[0]->outputs.size() ) {
        for (size_t i = 0; i < inputs[0]->outputs.size(); i++) {
            IRNode *candidate = inputs[0]->outputs[i];
            if (candidate->ival != ival) continue;
            if (candidate->fval != fval) continue;
            if (candidate->op != opcode) continue;
            if (candidate->type != t) continue;
            if (candidate->inputs.size() != inputs.size()) continue;
            bool inputsMatch = true;
            for (size_t j = 0; j < inputs.size(); j++) {
                if (candidate->inputs[j] != inputs[j]) inputsMatch = false;
            }
            // it's the same op on the same inputs, reuse the old node
            if (inputsMatch) return candidate;
        }
    }


    // We didn't see any reason to fuse or modify this op, so make a new node.
    return new IRNode(t, opcode, inputs, ival, fval);
}

// Any optimizations that must be done after generation is complete. Right now it just
// does the final sum rebalancing.
IRNode *IRNode::optimize() {
    IRNode *newNode = rebalanceSum();
    return newNode;
}        

// kill everything
void IRNode::clearAll() {
    IRNode::floatInstances.clear();
    IRNode::intInstances.clear();
    IRNode::varInstances.clear();
    for (size_t i = 0; i < IRNode::allNodes.size(); i++) {
        delete IRNode::allNodes[i];
    }
    IRNode::allNodes.clear();            
}

// type coercion
IRNode *IRNode::as(Type t) {
    if (t == type) return this;

    // insert new casting operators
    if (type == Int) {
        if (t == Float) 
            return make(IntToFloat, this);
        if (t == Bool)
            return make(NEQ, this, make(0));
    }

    if (type == Bool) {            
        if (t == Float) 
            return make(And, this, make(1.0f));
        if (t == Int) 
            return make(And, this, make(1));
    }

    if (type == Float) {
        if (t == Bool) 
            return make(NEQ, this, make(0.0f));
        if (t == Int) 
            return make(FloatToInt, this);
    }            

    panic("Casting to/from unknown type\n");
    return this;
            
}

void IRNode::printExp() {
    switch(op) {
    case Const:
        if (type == Float) printf("%f", fval);
        else printf("%d", ival);
        break;
    case VarX:
        printf("x");
        break;
    case VarY:
        printf("y");
        break;
    case VarT:
        printf("t");
        break;
    case VarC:
        printf("c");
        break;
    case UnboundVar:
        printf("<%x>", (int)this);
        break;
    case Plus:
        printf("(");
        inputs[0]->printExp();
        printf("+");
        inputs[1]->printExp();
        printf(")");
        break;
    case PlusImm:
        printf("(");
        inputs[0]->printExp();
        printf("+%d)", ival);
        break;
    case TimesImm:
        printf("(");
        inputs[0]->printExp();
        printf("*%d)", ival);
        break;
    case Minus:
        printf("(");
        inputs[0]->printExp();
        printf("-");
        inputs[1]->printExp();
        printf(")");
        break;
    case Times:
        printf("(");
        inputs[0]->printExp();
        printf("*");
        inputs[1]->printExp();
        printf(")");
        break;
    case Divide:
        printf("(");
        inputs[0]->printExp();
        printf("/");
        inputs[1]->printExp();
        printf(")");
        break;
    case LoadImm:
        printf("[");
        inputs[0]->printExp();
        printf("+%d]", ival);
        break;
    case Load:
        printf("[");
        inputs[0]->printExp();
        printf("]");
        break;
    default:
        if (inputs.size() == 0) {
            printf("%s", opname[op]);
        } else {
            printf("%s(", opname[op]); 
            inputs[0]->printExp();
            for (size_t i = 1; i < inputs.size(); i++) {
                printf(", ");
                inputs[1]->printExp();
            }
            printf(")");
        }
        break;
    }
}

void IRNode::print() {
            
    if (reg < 16)
        printf("r%d = ", reg);
    else 
        printf("xmm%d = ", reg-16);

    vector<string> args(inputs.size());
    char buf[32];
    for (size_t i = 0; i < inputs.size(); i++) {
        if (inputs[i]->reg < 0) 
            sprintf(buf, "%d", inputs[i]->ival);
        else if (inputs[i]->reg < 16)
            sprintf(buf, "r%d", inputs[i]->reg);
        else
            sprintf(buf, "xmm%d", inputs[i]->reg-16);
        args[i] += buf;
    }
            
    switch (op) {
    case Const:
        if (type == Float) printf("%f", fval);
        else printf("%d", ival);
        break;                
    case Plus:
        printf("%s + %s", args[0].c_str(), args[1].c_str());
        break;
    case Minus:
        printf("%s - %s", args[0].c_str(), args[1].c_str());
        break;
    case Times:
        printf("%s * %s", args[0].c_str(), args[1].c_str());
        break;
    case Divide:
        printf("%s / %s", args[0].c_str(), args[1].c_str());
        break;
    case PlusImm:
        printf("%s + %d", args[0].c_str(), ival);
        break;
    case TimesImm:
        printf("%s * %d", args[0].c_str(), ival);
        break;
    case LoadImm:
        printf("Load %s + %d", args[0].c_str(), ival);
        break;
    default:
        printf(opname[op]);
        for (size_t i = 0; i < args.size(); i++)
            printf(" %s", args[i].c_str());
        break;
    }

    printf("\n");
}

// make a new version of this IRNode with one of the variables replaced with a constant
IRNode *IRNode::substitute(OpCode var, int val) {
    if (op == var) return make(val);
    int dep = 0;
    switch (var) {
    case VarC:
        dep = DepC;
        break;
    case VarX:
        dep = DepX;
        break;
    case VarY:
        dep = DepY;
        break;
    case VarT:
        dep = DepT;
        break;
    default:
        panic("%s is not a variable!\n", opname[var]);
    }

    if (deps & dep) {
        // rebuild all the inputs
        vector<IRNode *> newInputs(inputs.size());
        for (size_t i = 0; i < newInputs.size(); i++) {
            newInputs[i] = inputs[i]->substitute(var, val);
        }
        return make(op, newInputs, ival, fval);
    } else {
        // no need to rebuild a subtree that doesn't depend on this variable
        return this;
    }
}

// bind unbound variables to x, y, t, and c
IRNode *IRNode::bind(IRNode *x, IRNode *y, IRNode *t, IRNode *c) {
    if (!(deps & DepUnbound)) return this;
    if (this == x) return IRNode::make(VarX);
    if (this == y) return IRNode::make(VarY);
    if (this == c) return IRNode::make(VarC);
    if (this == t) return IRNode::make(VarT);

    vector<IRNode *> newInputs(inputs.size());
    for (size_t i = 0; i < newInputs.size(); i++) {
        newInputs[i] = inputs[i]->bind(x, y, t, c);
    }

    IRNode *node = IRNode::make(op, newInputs, ival, fval);
    return node;
}
        
// Remove nodes that do not assist in the computation of these nodes
void IRNode::collectGarbage(vector<IRNode *> saved) {

    // mark all nodes for death
    for (size_t i = 0; i < allNodes.size(); i++) {
        allNodes[i]->marked = true;
    }

    // unmark those that are necessary for the computation of these nodes
    for (size_t i = 0; i < saved.size(); i++) {
        saved[i]->markDescendents(false);
    }

    vector<IRNode *> newAllNodes;
    map<float, IRNode *> newFloatInstances;
    map<int, IRNode *> newIntInstances;
    map<OpCode, IRNode *> newVarInstances;

    // save the unmarked nodes by migrating them to new data structures
    for (size_t i = 0; i < allNodes.size(); i++) {
        IRNode *n = allNodes[i];
        if (!n->marked) {
            newAllNodes.push_back(n);
            if (n->op == Const) {
                if (n->type == Float) 
                    newFloatInstances[n->fval] = n;
                else
                    newIntInstances[n->ival] = n;
            } else if (n->op == VarX ||
                       n->op == VarY ||
                       n->op == VarT ||
                       n->op == VarC) {
                newVarInstances[n->op] = n;
            }
        }
    }

    // delete the marked nodes
    for (size_t i = 0; i < allNodes.size(); i++) {
        IRNode *n = allNodes[i];
        if (n->marked) delete n;
    }

    allNodes.swap(newAllNodes);
    floatInstances.swap(newFloatInstances);
    intInstances.swap(newIntInstances);
    varInstances.swap(newVarInstances);
}

void IRNode::markDescendents(bool newMark) {
    if (marked == newMark) return;
    for (size_t i = 0; i < inputs.size(); i++) {
        inputs[i]->markDescendents(newMark);
    }
    marked = newMark;
}

IRNode *IRNode::rebalanceSum() {
    if (op != Plus && op != Minus && op != PlusImm) return this;
            
    // collect all the inputs
    vector<pair<IRNode *, bool> > terms;

    collectSum(terms);

    // extract out the const terms
    vector<pair<IRNode *, bool> > constTerms;
    vector<pair<IRNode *, bool> > nonConstTerms;
    for (size_t i = 0; i < terms.size(); i++) {
        if (terms[i].first->op == Const) {
            constTerms.push_back(terms[i]);
        } else {
            nonConstTerms.push_back(terms[i]);
        }
    }
            
    // sort the non-const terms by level
    for (size_t i = 0; i < nonConstTerms.size(); i++) {
        for (size_t j = i+1; j < nonConstTerms.size(); j++) {
            int li = nonConstTerms[i].first->level;
            int lj = nonConstTerms[j].first->level;
            if (li > lj) {
                pair<IRNode *, bool> tmp = nonConstTerms[i];
                nonConstTerms[i] = nonConstTerms[j];
                nonConstTerms[j] = tmp;
            }
        }
    }

    // remake the summation
    IRNode *t;
    bool tPos;
    t = nonConstTerms[0].first;
    tPos = nonConstTerms[0].second;

    // If we're building a float sum, the const term is innermost
    if (type == Float) {
        float c = 0.0f;
        for (size_t i = 0; i < constTerms.size(); i++) {
            if (constTerms[i].second) {
                c += constTerms[i].first->fval;
            } else {
                c -= constTerms[i].first->fval;
            }
        }
        if (c != 0.0f) {
            if (tPos) 
                t = make(Plus, make(c), t);
            else
                t = make(Minus, make(c), t);
        }
    }

    for (size_t i = 1; i < nonConstTerms.size(); i++) {
        bool nextPos = nonConstTerms[i].second;
        if (tPos == nextPos)
            t = make(Plus, t, nonConstTerms[i].first);
        else if (tPos) // and not nextPos
            t = make(Minus, t, nonConstTerms[i].first);
        else { // nextPos and not tPos
            tPos = true;
            t = make(Minus, nonConstTerms[i].first, t);
        }                    
    }

    // if we're building an int sum, the const term is
    // outermost so that loadimms can pick it up
    if (type == Int) {
        int c = 0;
        for (size_t i = 0; i < constTerms.size(); i++) {
            if (constTerms[i].second) {
                c += constTerms[i].first->ival;
            } else {
                c -= constTerms[i].first->ival;
            }
        }
        if (c != 0) {
            if (tPos) 
                t = make(PlusImm, t, NULL, NULL, NULL, c);
            else
                t = make(Minus, make(c), t);
        }
    }

    return t;
}


void IRNode::collectSum(vector<pair<IRNode *, bool> > &terms, bool positive) {
    if (op == Plus) {
        inputs[0]->collectSum(terms, positive);
        inputs[1]->collectSum(terms, positive);
    } else if (op == Minus) {
        inputs[0]->collectSum(terms, positive);
        inputs[1]->collectSum(terms, !positive);                
    } else if (op == PlusImm) {
        inputs[0]->collectSum(terms, positive);
        terms.push_back(make_pair(make(ival), true));
    } else {
        terms.push_back(make_pair(this, positive));
    }
}

// TODO: rebalance product

IRNode::IRNode(float v) {
    allNodes.push_back(this);
    op = Const;
    fval = v;
    ival = 0;
    deps = 0;
    reg = -1;
    level = 0;
    type = Float;
    width = 1;
}

IRNode::IRNode(int v) {
    allNodes.push_back(this);
    op = Const;
    ival = v;
    fval = 0.0f;
    deps = 0;
    reg = -1;
    level = 0;
    type = Int;
    width = 1;
}

IRNode::IRNode(Type t, OpCode opcode, 
       vector<IRNode *> input,
       int iv, float fv) {
    allNodes.push_back(this);

    ival = iv;
    fval = fv;

    for (size_t i = 0; i < input.size(); i++) 
        inputs.push_back(input[i]);

    type = t;
    width = 1;

    deps = 0;
    op = opcode;
    if (opcode == VarX) deps |= DepX;
    else if (opcode == VarY) deps |= DepY;
    else if (opcode == VarT) deps |= DepT;
    else if (opcode == VarC) deps |= DepC;
    else if (opcode == Load) deps |= DepMem;
    else if (opcode == UnboundVar) deps |= DepUnbound;

    for (size_t i = 0; i < inputs.size(); i++) {
        deps |= inputs[i]->deps;
        inputs[i]->outputs.push_back(this);
    }

    reg = -1;

    // compute the level based on deps
    if (deps & DepUnbound) level = 99;
    else if ((deps & DepC) || (deps & DepMem)) level = 4;
    else if (deps & DepX) level = 3;
    else if (deps & DepY) level = 2;
    else if (deps & DepT) level = 1;
    else level = 0;
}
