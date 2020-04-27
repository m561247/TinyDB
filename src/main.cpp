#include <string.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <signal.h>
#include "parser/execute.h"

extern "C" char run_parser(const char *input);

void free_all(int num) {
  execute_quit();
  printf("exit successful!");
  exit(0);
}

int main(int argc, char *argv[])
{
  std::string userInput;
  printf("WELCOME TO TINY DB!\n");
  printf(">> ");
  signal(SIGINT, free_all);

  while(getline(std::cin, userInput)) {
      if (userInput == "quit") {
        break;
      }
      run_parser(userInput.c_str());
      printf(">> ");
  }
  std::cout << "close" << std::endl;
  // close db
  execute_quit();
  return 0;
}
