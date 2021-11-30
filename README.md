# Tic Tac Toe game
## Homework description
Please design a pair of client-server Tic Tac Toe game programs with the following features:
1. Allow at least 2 clients to log in to the server at the same time.
2. Client users can list the users who have logged in.
3. The user of the client can choose which user to play game with and request the other party's consent.
4. If the opponent agrees, start the game.
5. The two sides can take turns to play until a winner or a tie is reached.
6. The logged in user can choose to log out.
<br>

## How to execute
```
make
sudo ./server
./client
```
You can look up the user's username and password through `account.txt`
<br>

## Additional features
1. `ls` will display whether the user is in-game.
2. Client users can't send a battle invitation to the users in the game.
3. The user of the client can refuse the invitation to play the game.
4. After the game starts, client usrs can only play the game or leave the room.
5. Support multi-person chat. 
<br>