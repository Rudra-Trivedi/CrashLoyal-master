  #include "Mob.h"

#include <memory>
#include <limits>
#include <stdlib.h>
#include <stdio.h>
#include "Building.h"
#include "Waypoint.h"
#include "GameState.h"
#include "Point.h"

int Mob::previousUUID;

Mob::Mob() 
	: pos(-10000.f,-10000.f)
	, nextWaypoint(NULL)
	, targetPosition(new Point)
	, state(MobState::Moving)
	, uuid(Mob::previousUUID + 1)
	, attackingNorth(true)
	, health(-1)
	, targetLocked(false)
	, target(NULL)
	, lastAttackTime(0)
{
	Mob::previousUUID += 1;
}

void Mob::Init(const Point& pos, bool attackingNorth)
{
	health = GetMaxHealth();
	this->pos = pos;
	this->attackingNorth = attackingNorth;
	findClosestWaypoint();
}

std::shared_ptr<Point> Mob::getPosition() {
	return std::make_shared<Point>(this->pos);
}

bool Mob::findClosestWaypoint() {
	std::shared_ptr<Waypoint> closestWP = GameState::waypoints[0];
	float smallestDist = std::numeric_limits<float>::max();

	for (std::shared_ptr<Waypoint> wp : GameState::waypoints) {
		//std::shared_ptr<Waypoint> wp = GameState::waypoints[i];
		// Filter out any waypoints that are "behind" us (behind is relative to attack dir
		// Remember y=0 is in the top left
		if (attackingNorth && wp->pos.y > this->pos.y) {
			continue;
		}
		else if ((!attackingNorth) && wp->pos.y < this->pos.y) {
			continue;
		}

		float dist = this->pos.dist(wp->pos);
		if (dist < smallestDist) {
			smallestDist = dist;
			closestWP = wp;
		}
	}
	std::shared_ptr<Point> newTarget = std::shared_ptr<Point>(new Point);
	this->targetPosition->x = closestWP->pos.x;
	this->targetPosition->y = closestWP->pos.y;
	this->nextWaypoint = closestWP;
	
	return true;
}

void Mob::moveTowards(std::shared_ptr<Point> moveTarget, double elapsedTime) {
	Point movementVector;
	movementVector.x = moveTarget->x - this->pos.x;
	movementVector.y = moveTarget->y - this->pos.y;
	movementVector.normalize();
	movementVector *= (float)this->GetSpeed();
	movementVector *= (float)elapsedTime;
	pos += movementVector;
}


void Mob::findNewTarget() {
	// Find a new valid target to move towards and update this mob
	// to start pathing towards it

	if (!findAndSetAttackableMob()) { findClosestWaypoint(); }
}

// Have this mob start aiming towards the provided target
// TODO: impliment true pathfinding here
void Mob::updateMoveTarget(std::shared_ptr<Point> target) {
	this->targetPosition->x = target->x;
	this->targetPosition->y = target->y;
}

void Mob::updateMoveTarget(Point target) {
	this->targetPosition->x = target.x;
	this->targetPosition->y = target.y;
}


// Movement related
//////////////////////////////////
// Combat related

int Mob::attack(int dmg) {
	this->health -= dmg;
	return health;
}

bool Mob::findAndSetAttackableMob() {
	// Find an attackable target that's in the same quardrant as this Mob
	// If a target is found, this function returns true
	// If a target is found then this Mob is updated to start attacking it
	for (std::shared_ptr<Mob> otherMob : GameState::mobs) {
		if (otherMob->attackingNorth == this->attackingNorth) { continue; }

		bool imLeft    = this->pos.x     < (SCREEN_WIDTH / 2);
		bool otherLeft = otherMob->pos.x < (SCREEN_WIDTH / 2);

		bool imTop    = this->pos.y     < (SCREEN_HEIGHT / 2);
		bool otherTop = otherMob->pos.y < (SCREEN_HEIGHT / 2);
		if ((imLeft == otherLeft) && (imTop == otherTop)) {
			// If we're in the same quardrant as the otherMob
			// Mark it as the new target
			this->setAttackTarget(otherMob);
			return true;
		}
	}
	return false;
}

// TODO Move this somewhere better like a utility class
int randomNumber(int minValue, int maxValue) {
	// Returns a random number between [min, max). Min is inclusive, max is not.
	return (rand() % maxValue) + minValue;
}

void Mob::setAttackTarget(std::shared_ptr<Attackable> newTarget) {
	this->state = MobState::Attacking;
	target = newTarget;
}

bool Mob::targetInRange() {
	float range = this->GetSize(); // TODO: change this for ranged units
	float totalSize = range + target->GetSize();
	return this->pos.insideOf(*(target->getPosition()), totalSize);
}
// Combat related
////////////////////////////////////////////////////////////
// Collisions

// PROJECT 3: 
//  1) return a vector of mobs that we're colliding with
//  2) handle collision with towers & river 
std::vector<std::shared_ptr<Mob>> Mob::checkCollision() {
	std::vector<std::shared_ptr<Mob>> othermobvec;

	if (this->pos.y > ((GAME_GRID_HEIGHT / 2) - 1.5) && this->pos.y <((GAME_GRID_HEIGHT / 2) + 1.5))
	{
		if (!(this->pos.x >= ((GAME_GRID_WIDTH / 4.0) - 3) && this->pos.x <= ((GAME_GRID_WIDTH / 4.0) + 3)) && !((this->pos.x >= ((GAME_GRID_WIDTH - (GAME_GRID_WIDTH / 4.0)) - 3)) && (this->pos.x <= ((GAME_GRID_WIDTH - (GAME_GRID_WIDTH / 4.0)) + 3))))
		{
			std::cout << "Collsion Check" << std::endl;
			this->pos.y = ((GAME_GRID_HEIGHT / 2) + 1.5) + 1.0f;
		}
	}

	for (std::shared_ptr<Building> otherBuilding : GameState::buildings)
	{
		if (this->pos.insideOf(*(otherBuilding->getPosition()), otherBuilding->GetSize()))
		{
			if (this->IsAttackingNorth())
			{
				
				this->pos.y = otherBuilding->getPoint().y + otherBuilding->GetSize();

			}

			else
			{
				this->pos.y = otherBuilding->getPoint().y - otherBuilding->GetSize();
			}
		}
	}


	for (std::shared_ptr<Mob> otherMob : GameState::mobs) {
		// don't collide with yourself
		if ((this->sameMob(otherMob)))
		{ 
			continue;
		}

		else
		{
			othermobvec.push_back(otherMob);
		}

		// PROJECT 3: YOUR CODE CHECKING FOR A COLLISION GOES HERE
	}
	//return std::shared_ptr<Mob>(nullptr);
	return othermobvec;
	
}

void Mob::processCollision(std::shared_ptr<Mob> otherMob, double elapsedTime) {
	
	// NEWER APPROACH
	//Point colpointtl;
	//Point colpointtr;
	//Point colpointbl;
	//Point colpointbr;
	//colpointtl.x = this->pos.x - this->GetSize();
	//colpointtl.y = this->pos.y + this->GetSize();
	//colpointtr.x = this->pos.x + this->GetSize();
	//colpointtr.y = this->pos.y + this->GetSize();
	//colpointbl.x = this->pos.x - this->GetSize();
	//colpointbl.y = this->pos.y - this->GetSize();
	//colpointbr.x = this->pos.x + this->GetSize();
	//colpointbr.y = this->pos.y - this->GetSize();


	//if ((colpointtl).insideOf(*(otherMob->getPosition()), otherMob->GetSize()))
	//{
	//	if (this->GetMass() > otherMob->GetMass())
	//	{
	//		
	//		otherMob->pos.x = this->getPosition()->x + this->GetSize() ;
	//		otherMob->pos.y = this->getPosition()->y - this->GetSize();
	//		
	//	}

	//	else if(this->GetMass() == otherMob->GetMass())
	//	{
	//		this->pos.x += this->GetSize();
	//		this->pos.y -= this->GetSize();
	//		/*this->pos.x += otherMob->GetSize();
	//		this->pos.y += otherMob->GetSize();*/
	//	}

	//	/*else
	//	{
	//		otherMob->pos.x += this->GetSize();
	//		otherMob->pos.y += this->GetSize();
	//	}*/
	//}

	//else if ((colpointtr).insideOf(*(otherMob->getPosition()), otherMob->GetSize()))
	//{
	//	if (this->GetMass() > otherMob->GetMass())
	//	{

	//		otherMob->pos.x = this->getPosition()->x + this->GetSize();
	//		otherMob->pos.y = this->getPosition()->y - this->GetSize();

	//	}

	//	else if (this->GetMass() == otherMob->GetMass())
	//	{
	//		this->pos.x += this->GetSize();
	//		this->pos.y -= this->GetSize();
	//		/*this->pos.x += otherMob->GetSize();
	//		this->pos.y += otherMob->GetSize();*/
	//	}

	//	//else
	//	//{
	//	//	this->pos.x = otherMob->getPosition()->x + otherMob->GetSize();
	//	//	this->pos.y = otherMob->getPosition()->y - otherMob->GetSize();
	//	//	/*this->pos.x += otherMob->GetSize();
	//	//	this->pos.y += otherMob->GetSize();*/
	//	//}

	//	/*else
	//	{
	//		otherMob->pos.x += this->GetSize();
	//		otherMob->pos.y += this->GetSize();
	//	}*/
	//}

	//else if ((colpointbl).insideOf(*(otherMob->getPosition()), otherMob->GetSize()))
	//{
	//	if (this->GetMass() > otherMob->GetMass())
	//	{

	//		otherMob->pos.x = this->getPosition()->x + this->GetSize();
	//		otherMob->pos.y = this->getPosition()->y - this->GetSize();

	//	}

	//	else if (this->GetMass() == otherMob->GetMass())
	//	{
	//		this->pos.x += this->GetSize();
	//		this->pos.y -= this->GetSize();
	//		/*this->pos.x += otherMob->GetSize();
	//		this->pos.y += otherMob->GetSize();*/
	//	}

	//	/*else
	//	{
	//		otherMob->pos.x += this->GetSize();
	//		otherMob->pos.y += this->GetSize();
	//	}*/
	//}

	//else if ((colpointbr).insideOf(*(otherMob->getPosition()), otherMob->GetSize()))
	//{
	//	if (this->GetMass() > otherMob->GetMass())
	//	{

	//		otherMob->pos.x = (this->getPosition()->x + this->GetSize()) ;
	//		otherMob->pos.y = this->getPosition()->y - this->GetSize();

	//	}

	//	else if (this->GetMass() == otherMob->GetMass())
	//	{
	//		this->pos.x += this->GetSize();
	//		this->pos.y -= this->GetSize();
	//		/*this->pos.x += otherMob->GetSize();
	//		this->pos.y += otherMob->GetSize();*/
	//	}

	//	/*else
	//	{
	//		otherMob->pos.x += this->GetSize();
	//		otherMob->pos.y += this->GetSize();
	//	}*/
	//}


	//older one -----------------------------------------------------------------------------------------------
	//// Get the Position of the other mob 
	//std::shared_ptr<Point> OtherMobPos = otherMob->getPosition();
	//float x = OtherMobPos->x;
	//float y = OtherMobPos->y;
	//float size = otherMob->GetSize();
	//// Getting the coordinates of the corners of the othermob 
	///*Point TL(x - (size / 2), y - (size / 2));
	//Point BL(x - (size / 2), y + (size / 2));
	//Point TR(x + (size / 2), y - (size / 2));
	//Point BR(x + (size / 2), y + (size / 2));*/

	//Point TL(x - (size / 2), y + (size / 2));
	//Point BL(x - (size / 2), y - (size / 2));
	//Point TR(x + (size / 2), y + (size / 2));
	//Point BR(x + (size / 2), y - (size / 2));

	//// Getting the coordinates of this mob's corners
	///*Point thisTL(this->getPosition()->x - (this->GetSize() / 2), this->getPosition()->y - (this->GetSize() / 2));
	//Point thisBL(this->getPosition()->x - (this->GetSize() / 2), this->getPosition()->y + (this->GetSize() / 2));
	//Point thisTR(this->getPosition()->x + (this->GetSize() / 2), this->getPosition()->y - (this->GetSize() / 2));
	//Point thisBR(this->getPosition()->x + (this->GetSize() / 2), this->getPosition()->y + (this->GetSize() / 2));*/

	//Point thisTL(this->getPosition()->x - (this->GetSize() / 2), this->getPosition()->y + (this->GetSize() / 2));
	//Point thisBL(this->getPosition()->x - (this->GetSize() / 2), this->getPosition()->y - (this->GetSize() / 2));
	//Point thisTR(this->getPosition()->x + (this->GetSize() / 2), this->getPosition()->y + (this->GetSize() / 2));
	//Point thisBR(this->getPosition()->x + (this->GetSize() / 2), this->getPosition()->y - (this->GetSize() / 2));


	//if (TL.x <= thisTR.x && TL.x >= thisTL.x && TL.y <= thisTR.y && TL.y >= thisBR.y)
	//{
	//	Point other(((this->getPosition()->x) + ((this->GetSize()) / 2)), this->getPosition()->y);
	//	float dis = TL.dist(other);
	//	this->pos.x -= dis + (dis / 2);
	//	this->pos.y += dis + (dis / 2);

	//	std::cout<< "Top right collision" << "COLLIDING WITH OTHER <<<<<<<<<<<<<<" << std::endl;
	//
	//}
	//
	//else if (TR.x >= thisTL.x && TR.x <= thisTR.x && TR.y <= thisTL.y && TR.y >= thisBL.y)
	//{
	//	Point other(((this->getPosition()->x) - ((this->GetSize()) / 2)), this->getPosition()->y);
	//	float dis = TR.dist(other);
	//	this->pos.x += dis + (dis / 2);
	//	this->pos.y += dis + (dis / 2);

	//	std::cout<<"Top left Collision" << "COLLIDING WITH OTHER <<<<<<<<<<<<<<" << std::endl;
	//}

	//else if (BL.x >= thisTL.x && BL.x <= thisTR.x && BL.y <= thisTL.y && BL.y >= thisBL.y)
	//{
	//	Point other(((this->getPosition()->x) + ((this->GetSize()) / 2)), this->getPosition()->y);
	//	float dis = BL.dist(other);
	//	this->pos.x -= dis + (dis / 2);
	//	this->pos.y -= dis + (dis / 2);

	//	std::cout << "Top Bottom Left Collision" << "COLLIDING WITH OTHER <<<<<<<<<<<<<<" << std::endl;
	//}


	//else if (BR.x >= thisTL.x && TR.x <= thisTR.x && BR.y <= thisTL.y && BR.y >= thisBL.y)
	//{
	//	Point other(((this->getPosition()->x) - ((this->GetSize()) / 2)), this->getPosition()->y);
	//	float dis = BR.dist(other);
	//	this->pos.x += dis + (dis / 2);
	//	this->pos.y -= dis + (dis/2);
	//	std::cout << "Bottom Right Collision" << "COLLIDING WITH OTHER <<<<<<<<<<<<<<" << std::endl;
	//}

	//
	//

	// PROJECT 3: YOUR COLLISION HANDLING CODE GOES HERE

	std::shared_ptr<Point> OtherMobPos = otherMob->getPosition();
	float x = OtherMobPos->x;
	float y = OtherMobPos->y;
	float size = otherMob->GetSize();
	int MassSelect = 0;
	if (this->GetMass() > otherMob->GetMass())
	{
		MassSelect = 1;
	}

	else if (this->GetMass() == otherMob->GetMass())
	{
		MassSelect = 2;
	}

	// Getting the coordinates of the corners of the othermob 
	/*Point TL(x - (size / 2), y - (size / 2));
	Point BL(x - (size / 2), y + (size / 2));
	Point TR(x + (size / 2), y - (size / 2));
	Point BR(x + (size / 2), y + (size / 2));*/

	Point TL(x - (size / 2), y + (size / 2));
	Point BL(x - (size / 2), y - (size / 2));
	Point TR(x + (size / 2), y + (size / 2));
	Point BR(x + (size / 2), y - (size / 2));

	// Getting the coordinates of this mob's corners
	/*Point thisTL(this->getPosition()->x - (this->GetSize() / 2), this->getPosition()->y - (this->GetSize() / 2));
	Point thisBL(this->getPosition()->x - (this->GetSize() / 2), this->getPosition()->y + (this->GetSize() / 2));
	Point thisTR(this->getPosition()->x + (this->GetSize() / 2), this->getPosition()->y - (this->GetSize() / 2));
	Point thisBR(this->getPosition()->x + (this->GetSize() / 2), this->getPosition()->y + (this->GetSize() / 2));*/

	Point thisTL(this->getPosition()->x - (this->GetSize() / 2), this->getPosition()->y + (this->GetSize() / 2));
	Point thisBL(this->getPosition()->x - (this->GetSize() / 2), this->getPosition()->y - (this->GetSize() / 2));
	Point thisTR(this->getPosition()->x + (this->GetSize() / 2), this->getPosition()->y + (this->GetSize() / 2));
	Point thisBR(this->getPosition()->x + (this->GetSize() / 2), this->getPosition()->y - (this->GetSize() / 2));


	if (TL.insideOf(*(this->getPosition()), this->GetSize()))
	{
		Point other(((this->getPosition()->x) + ((this->GetSize()) / 2)), this->getPosition()->y);
		float dis = TL.dist(other);
		if (MassSelect == 1)
		{
			otherMob->pos.x -= dis + (dis / 2);
			otherMob->pos.y += dis + (dis / 2);
		}
		else if (MassSelect == 2)
		{
			this->pos.x -= dis + (dis / 2);
			this->pos.y += dis + (dis / 2);
		}

		std::cout<< "Top right collision" << "COLLIDING WITH OTHER <<<<<<<<<<<<<<" << std::endl;
	
	}
	
	else if (TR.insideOf(*(this->getPosition()), this->GetSize()))
	{
		Point other(((this->getPosition()->x) - ((this->GetSize()) / 2)), this->getPosition()->y);
		float dis = TR.dist(other);

		if (MassSelect == 1)
		{
			otherMob->pos.x += dis + (dis / 2);
			otherMob->pos.y += dis + (dis / 2);
		}

		else if (MassSelect == 2)
		{
			this->pos.x += dis + (dis / 2);
			this->pos.y += dis + (dis / 2);
		}

		/*this->pos.x += dis + (dis / 2);
		this->pos.y += dis + (dis / 2);*/

		std::cout<<"Top left Collision" << "COLLIDING WITH OTHER <<<<<<<<<<<<<<" << std::endl;
	}

	else if (BL.insideOf(*(this->getPosition()), this->GetSize()))
	{
		Point other(((this->getPosition()->x) + ((this->GetSize()) / 2)), this->getPosition()->y);
		float dis = BL.dist(other);

		if (MassSelect == 1)
		{
			otherMob->pos.x -= dis + (dis / 2);
			otherMob->pos.y -= dis + (dis / 2);
		}

		else if (MassSelect == 2)
		{
			this->pos.x -= dis + (dis / 2);
			this->pos.y -= dis + (dis / 2);
		}

		/*this->pos.x -= dis + (dis / 2);
		this->pos.y -= dis + (dis / 2);*/

		std::cout << "Top Bottom Left Collision" << "COLLIDING WITH OTHER <<<<<<<<<<<<<<" << std::endl;
	}


	else if (BR.insideOf(*(this->getPosition()), this->GetSize()))
	{
		Point other(((this->getPosition()->x) - ((this->GetSize()) / 2)), this->getPosition()->y);
		float dis = BR.dist(other);
		if (MassSelect == 1)
		{
			otherMob->pos.x += dis + (dis / 2);
			otherMob->pos.y -= dis + (dis / 2);
		}

		else if (MassSelect == 2)
		{
			this->pos.x += dis + (dis / 2);
			this->pos.y -= dis + (dis / 2);
		}

		/*this->pos.x += dis + (dis / 2);
		this->pos.y -= dis + (dis/2);*/
		std::cout << "Bottom Right Collision" << "COLLIDING WITH OTHER <<<<<<<<<<<<<<" << std::endl;
	}
}

// Collisions
///////////////////////////////////////////////
// Procedures

void Mob::attackProcedure(double elapsedTime) {
	if (this->target == nullptr || this->target->isDead()) {
		this->targetLocked = false;
		this->target = nullptr;
		this->state = MobState::Moving;
		return;
	}

	if (targetInRange()) {
		if (this->lastAttackTime >= this->GetAttackTime()) {
			// If our last attack was longer ago than our cooldown
			this->target->attack(this->GetDamage());
			this->lastAttackTime = 0; // lastAttackTime is incremented in the main update function
			return;
		}
	}
	else {
		// If the target is not in range
		moveTowards(target->getPosition(), elapsedTime);
	}
}

void Mob::moveProcedure(double elapsedTime) {
	if (targetPosition) {
		moveTowards(targetPosition, elapsedTime);

		// Check for collisions
		if (this->nextWaypoint->pos.insideOf(this->pos, (this->GetSize() + WAYPOINT_SIZE))) {
			std::shared_ptr<Waypoint> trueNextWP = this->attackingNorth ?
												   this->nextWaypoint->upNeighbor :
												   this->nextWaypoint->downNeighbor;
			setNewWaypoint(trueNextWP);
		}

		// PROJECT 3: You should not change this code very much, but this is where your 
		// collision code will be called from
		std::vector<std::shared_ptr<Mob>> otherMob = this->checkCollision();
		std::vector<std::shared_ptr<Mob>>::iterator ptr;
		if (!otherMob.empty()) {
			for (ptr = otherMob.begin(); ptr < otherMob.end(); ptr++)
			{
				this->processCollision(*ptr, elapsedTime);
			}
		}

		// Fighting otherMob takes priority always
		findAndSetAttackableMob();

	} else {
		// if targetPosition is nullptr
		findNewTarget();
	}
}

void Mob::update(double elapsedTime) {

	switch (this->state) {
	case MobState::Attacking:
		this->attackProcedure(elapsedTime);
		break;
	case MobState::Moving:
	default:
		this->moveProcedure(elapsedTime);
		break;
	}

	this->lastAttackTime += (float)elapsedTime;
}
