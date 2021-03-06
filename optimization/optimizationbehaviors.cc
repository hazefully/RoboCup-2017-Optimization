#include "optimizationbehaviors.h"
#include <fstream>
#define walkFront(distance) goToTarget(VecPosition(me.getX()+distance, me.getY(), me.getZ()))
#define walkFrontInf() goToTargetRelative(VecPosition(1, 0, 0), 0)
#define walkBackwards() goToTargetRelative(VecPosition(-1, 0, 0), 0)
#define walkRightLat() goToTargetRelative(VecPosition(0, -1 ,0), 0)
#define walkLeftLat() goToTargetRelative(VecPosition(0, 1, 0), 0)
#define walkRight(distance) goToTarget(VecPosition(me.getX(), me.getY()+distance, me.getZ()))
#define walkLeft(distance) goToTarget(VecPosition(me.getX(), me.getY()+distance, me.getZ()))
#define walkBackwardsTurn(distance) goToTarget(VecPosition(me.getX()-distance, me.getY(), me.getZ()))
#define walk45up(distance) goToTarget(VecPosition(me.getX()+distance, me.getY()-distance, me.getZ()))
#define walk45down(distance) goToTarget(VecPosition(me.getX()-distance, me.getY()+distance, me.getZ()))
#define walkSinUp()  goToTargetRelative(VecPosition(1, 1 ,0), 0)
#define walkSinDown()  goToTargetRelative(VecPosition(1, -1 ,0), 0)
#define turnCCW() goToTargetRelative(VecPosition(0, 1, 0), 90)
#define turnCW() goToTargetRelative(VecPosition(0, -1, 0), -90)
#define walkInACircle() goToTargetRelative(VecPosition(1, 0, 0), 10)
/*
 *
 *
 * Fixed Kick optimization agent
 *
 *
 */
OptimizationBehaviorFixedKick::OptimizationBehaviorFixedKick(

const std::string teamName, int uNum, const map<string, string>& namedParams_,
		const string& rsg_, const string& outputFile_) :
		NaoBehavior(teamName, uNum, namedParams_, rsg_), outputFile(
				outputFile_), kick(0), INIT_WAIT_TIME(3.0) {
	initKick();
}

void OptimizationBehaviorFixedKick::beam(double& beamX, double& beamY,
		double& beamAngle) {
	beamX = atof(namedParams.find("kick_xoffset")->second.c_str());
	beamY = atof(namedParams.find("kick_yoffset")->second.c_str());
	beamAngle = atof(namedParams.find("kick_angle")->second.c_str());
}

SkillType OptimizationBehaviorFixedKick::selectSkill() {
	double time = worldModel->getTime();

	if (timeStart < 0) {
		initKick();
		return SKILL_STAND;
	}

	// Wait a bit before attempting kick
	if (time - timeStart <= INIT_WAIT_TIME) {
		return SKILL_STAND;
	}

	if (!hasKicked) {
		hasKicked = true;
		return SKILL_KICK_LEFT_LEG; // The kick skill that we're optimizing
	}

	return SKILL_STAND;
}

void OptimizationBehaviorFixedKick::updateFitness() {
	static double totalFitness = 0.0;
	if (kick == 10) {
		writeFitnessToOutputFile(totalFitness / (double(kick)));
		return;
	}

	double time = worldModel->getTime();
	VecPosition meTruth = worldModel->getMyPositionGroundTruth();
	meTruth.setZ(0);

	if (time - timeStart <= INIT_WAIT_TIME) {
		return;
	}

	static bool failedLastBeamCheck = false;
	if (!beamChecked) {
		beamChecked = true;
		meTruth.setZ(0);
		double beamX, beamY, beamAngle;
		beam(beamX, beamY, beamAngle);
		VecPosition meDesired = VecPosition(beamX, beamY, 0);
		double distance = meTruth.getDistanceTo(meDesired);
		double angle = worldModel->getMyAngDegGroundTruth();
		VecPosition ballPos = worldModel->getBallGroundTruth();
		double ballDistance = ballPos.getMagnitude();
		failedLastBeamCheck = false;
		string msg = "(playMode KickOff_Left)";
		setMonMessage(msg);
	}

	if (!hasKicked && worldModel->getPlayMode() == PM_PLAY_ON) {
		ranIntoBall = true;
	}

	if (!hasKicked) {
		return;
	}

	VecPosition ballTruth = worldModel->getBallGroundTruth();
	if (ballTruth.getX() < -.25) {
		backwards = true;
	}

	if (worldModel->isFallen()) {
		fallen = true;
	}

	if (time - (timeStart + INIT_WAIT_TIME) > 15
			&& !isBallMoving(this->worldModel)) {
		double angleOffset = abs(
				VecPosition(0, 0, 0).getAngleBetweenPoints(
						VecPosition(20, 0, 0), ballTruth));
		double distance = ballTruth.getX();
		double fitness = distance;

		if (backwards || distance <= 0.1 || ranIntoBall) {
			fitness = -100;
		}
		cout << "kick " << kick << " = " << fitness << endl;

		totalFitness += fitness;
		kick++;
		initKick();
		return;
	}
}

void OptimizationBehaviorFixedKick::initKick() {
	hasKicked = false;
	beamChecked = false;
	backwards = false;
	ranIntoBall = false;
	timeStart = worldModel->getTime();
	initialized = false;
	initBeamed = false;
	fallen = false;
	resetSkills();

	// Beam agent and ball
	double beamX, beamY, beamAngle;
	beam(beamX, beamY, beamAngle);
	VecPosition beamPos = VecPosition(beamX, beamY, 0);
	string msg = "(playMode BeforeKickOff)";
	setMonMessage(msg);
}

void OptimizationBehaviorFixedKick::writeFitnessToOutputFile(double fitness) {
	static bool written = false;
	if (!written) {
		fstream file;
		file.open(outputFile.c_str(), ios::out);
		file << fitness << endl;
		file.close();
		written = true;
		//string msg = "(killsim)";
		//setMonMessage(msg);
	}
}

/* Checks if the ball is currently moving */
bool isBallMoving(const WorldModel *worldModel) {
	static VecPosition lastBall = worldModel->getBallGroundTruth();
	static double lastTime = worldModel->getTime();

	double thisTime = worldModel->getTime();
	VecPosition thisBall = worldModel->getBallGroundTruth();

	thisBall.setZ(0);
	lastBall.setZ(0);

	if (thisBall.getDistanceTo(lastBall) > 0.01) {
		// the ball moved!
		lastBall = thisBall;
		lastTime = thisTime;
		return true;
	}

	if (thisTime - lastTime < 0.5) {
		// not sure yet if the ball has settled
		return true;
	} else {
		return false;
	}
}

void writeToOutputFile(const string &filename, const string &output) {
//  static bool written = false;
//  assert(!written);
	//  LOG(output);
	fstream file;
	file.open(filename.c_str(), ios::out);
	file << output;
	file.close();
//  written = true;
}

/*
 *
 *
 * WALK FORWARD OPTIMIZATION AGENT
 *
 *
 *
 */
OptimizationBehaviorWalkForward::OptimizationBehaviorWalkForward(
		const std::string teamName, int uNum,
		const map<string, string>& namedParams_, const string& rsg_,
		const string& outputFile_) :
		NaoBehavior(teamName, uNum, namedParams_, rsg_), outputFile(outputFile_) {
	INIT_WAIT = 1;
	angle = 0;
	run = 0;
	for (int i = 0; i < 30; i++)
		DIS[i] = 0.0, startSet[i] = false;
	sumOfWeights = sumOfWeightsOverDistance = 0;
	expectedPerTarget = 5.0;
	DIST_TO_WALK = 2.5;
	totalWalkDist = 0;
	flip = 1;
	START_POSITION_X[0] = -HALF_FIELD_X + 7;
	START_POSITION_Y[0] = -HALF_FIELD_Y + 3;
	startCalc = false;

	for (int i = 0; i < 5; i++)
		START_POSITION_X[i] = -HALF_FIELD_X + 7, START_POSITION_Y[i] =
				-HALF_FIELD_Y + 3;
//	START_POSITION_Y[3] = 0;
	spiralDirection = args(VecPosition(0, 0, 0), 0);
//	START_POSITION_X[1] = -6;
//	START_POSITION_Y[1] = 0;
//	START_POSITION_X[2] = -1;
//	START_POSITION_Y[2] = 0;
//	START_POSITION_X[3] = -HALF_FIELD_X + 7;
//	START_POSITION_Y[3] = -HALF_FIELD_Y + 3;
//	START_POSITION_X[4] = -HALF_FIELD_X + 3;
//	START_POSITION_Y[4] = 0;
	COST_OF_FALL = 5;
	maxP = VecPosition(-50, -50, -50);
	// Use ground truth localization for behavior
	worldModel->setUseGroundTruthDataForLocalization(true);
	for (int i = 0; i < 12; i++) {
		movements[i].clear();
		movementsTypes[i].clear();
	}
	populateMovements();

	init();
}

void OptimizationBehaviorWalkForward::init() {

	startTime = worldModel->getTime();
	initialized = false;
	initBeamed = false;
	flip = true;
	hasFallen = false;
	falls = 0;
	oldTarget = 0;
	beamChecked = false;
	x = false;
	for (int i = 0; i < 30; i++) {
		DIS[i] = 0.0;
		startSet[i] = false;
		targetSet[i] = false;
		orientation[i] = 720;
		startt[i] = VecPosition(-2, 0, 0);

	}
	for (int i = 0; i < 4; i++)
		costSet[i] = false;
//	moveTypes[0] = 3;

	times[0] = 15.0;
	moveTypes[0] = 0;
	times[1] = 16.0;
	moveTypes[1] = 5;
	times[2] = 22.0;
	moveTypes[2] = 0;
	times[3] = 23.0;
	moveTypes[3] = 5;
	times[4] = 27.0;
	moveTypes[4] = 0;
	times[5] = 28.0;
	moveTypes[5] = 5;
	times[6] = 30.0;
	moveTypes[6] = 4;
	times[7] = 40.0;
	moveTypes[7] = 0;
	times[8] = 41.0;
	moveTypes[8] = 5;
	times[9] = 45.0;
	moveTypes[9] = 4;

	times[10] = 50.0;
	moveTypes[10] = 0;
	times[11] = 51.0;
	moveTypes[11] = 5;

	times[12] = 66.0;
	moveTypes[12] = 1;

	times[13] = 67.0;
	moveTypes[13] = 5;
	times[14] = 72.0;
	moveTypes[14] = 1;

	times[15] = 73.0;
	moveTypes[15] = 5;

	times[16] = 78.0;
	moveTypes[16] = 0;
	times[17] = 88.0;
	moveTypes[17] = 2;
	times[18] = 98.0;
	moveTypes[18] = 2;
	times[19] = 88.0;
	moveTypes[19] = 5;
	times[20] = 109.0;
	moveTypes[20] = 0;
	times[21] = 110.0;
	moveTypes[21] = 0;
	times[22] = 120.0;
	moveTypes[22] = 0;
	times[23] = 121.0;
	moveTypes[23] = 0;
	times[24] = 131.0;
	moveTypes[24] = 0;
	times[25] = 132.0;
	moveTypes[25] = 0;
	times[26] = 142.0;
	moveTypes[26] = 0;
	times[27] = 155.0;
	moveTypes[27] = 3;

	times[28] = 154.0;
	moveTypes[28] = 0;
	times[29] = 160.0;
	moveTypes[29] = 3;

	flipTime = 0;
	standing = false;
	angle = 0;
	startCalc = false;
	turned = false;

	randomTarget = VecPosition(HALF_FIELD_X, HALF_FIELD_Y, 0);
	targetStartTime = startTime + INIT_WAIT;
	double currentTime = worldModel->getTime();
	randomIsSetTime = currentTime - 0.5;
	double x, y, z;
	beam(x, y, z);
	string msg = "(playMode BeforeKickOff)";
	setMonMessage(msg);
}
void OptimizationBehaviorWalkForward::populateMovements() {
	args forward = args(VecPosition(1, 0, 0), 0);
	args backwards = args(VecPosition(-1, 0, 0), 0);
	args left_lat = args(VecPosition(0, 1, 0), 0);
	args right_lat = args(VecPosition(0, -1, 0), 0);
	args circle = args(VecPosition(1, 0, 0), 7);
	args weave45_for = args(VecPosition(1, 1, 0), 45);
	args weave45_back = args(VecPosition(-1, -1, 0), 45);
	args downRight = args(VecPosition(1, -1, 0), 0);
	spiralDirection = args(VecPosition(1, 0, 0), 200);
	args turn_counter_clockwise = args(VecPosition(0, 1, 0), 90);
	args turn_clockwise = args(VecPosition(0, -1, 0), -90);
	args stop = args(true);

	movements[0].push_back(pair<args, double>(forward, 6));
	movementsTypes[0].push_back(0);

	movements[0].push_back(pair<args, double>(backwards, 15.0));
	movementsTypes[0].push_back(0);

	weights[0] = 5;

	movements[1].push_back(pair<args, double>(right_lat, 7));
	movementsTypes[1].push_back(4);
	movements[1].push_back(pair<args, double>(left_lat, 14));
	movementsTypes[1].push_back(4);
	movements[1].push_back(pair<args, double>(stop, 15));
	movementsTypes[1].push_back(5);

	weights[1] = 4;

	movements[2].push_back(pair<args, double>(circle, 15));
	movementsTypes[2].push_back(1);

	weights[2] = 1;

	movements[3].push_back(pair<args, double>(forward, 15));
	movementsTypes[3].push_back(0);

	weights[3] = 5;

	movements[4].push_back(pair<args, double>(turn_counter_clockwise, 7.5));
	movementsTypes[4].push_back(3);
	movements[4].push_back(pair<args, double>(turn_clockwise, 15));
	movementsTypes[4].push_back(3);

	weights[4] = 2;
//	movements[3].push_back(pair<args, double>(backwards, 10));
//	movementsTypes[3].push_back(0);
//	movements[3].push_back(pair<args, double>(forward, 14));
//	movementsTypes[3].push_back(0);
//	movements[3].push_back(pair<args, double>(stop, 15));
//	movementsTypes[3].push_back(5);
//
//	movements[4].push_back(pair<args, double>(circle, 15));
//	movementsTypes[4].push_back(1);

	for (double x = 0.2; x <= 15.0 - 0.2; x += 0.4) {
		movements[5].push_back(pair<args, double>(forward, x));
		movementsTypes[5].push_back(0);
		movements[5].push_back(pair<args, double>(left_lat, x + 0.2));
		movementsTypes[5].push_back(4);
	}
	for (double x = 0; x <= 15.0 - 0.1; x += 0.1) {
		movements[6].push_back(pair<args, double>(forward, x + 0.05));
		movementsTypes[6].push_back(0);
		movements[6].push_back(
				pair<args, double>(turn_counter_clockwise, x + 0.1));
		movementsTypes[6].push_back(3);
	}
	weights[5] = 3;
	weights[6] = 2;
	weights[7] = 1;
//	circle.angle = 15;
//	movements[6].push_back(pair<args, double>(circle, 15));
//	movementsTypes[6].push_back(1);
//	movements[5].push_back(pair<args, double>(forward, 4));
//	movementsTypes[5].push_back(0);
//	movements[5].push_back(pair<args, double>(stop, 5));
//	movementsTypes[5].push_back(5);
//	movements[5].push_back(pair<args, double>(right_lat, 12));
//	movementsTypes[5].push_back(4);
//	movements[5].push_back(pair<args, double>(stop, 13));
//	movementsTypes[5].push_back(5);
//	movements[5].push_back(pair<args, double>(left_lat, 15));
//	movementsTypes[5].push_back(4);

//
//
//	movements[7].push_back(pair<args, double>(forward, 2));
//	movementsTypes[7].push_back(0);
//	movements[7].push_back(pair<args, double>(right_lat, 4));
//	movementsTypes[7].push_back(4);
//	movements[7].push_back(pair<args, double>(left_lat, 6));
//	movementsTypes[7].push_back(4);
//	movements[7].push_back(pair<args, double>(backwards, 8));
//	movementsTypes[7].push_back(0);
//	movements[7].push_back(pair<args, double>(right_lat, 10));
//	movementsTypes[7].push_back(4);
//	movements[7].push_back(pair<args, double>(left_lat, 14));
//	movementsTypes[7].push_back(4);
//	movements[7].push_back(pair<args, double>(stop, 15));
//	movementsTypes[7].push_back(5);
//
//	movements[8].push_back(pair<args, double>(right_lat, 7));
//	movementsTypes[8].push_back(4);
//
//	movements[8].push_back(pair<args, double>(left_lat, 14));
//	movementsTypes[8].push_back(4);
//
//	movements[8].push_back(pair<args, double>(stop, 15));
//	movementsTypes[8].push_back(5);

}
double OptimizationBehaviorWalkForward::fitnessCircle(VecPosition start) {
	double ret = 0;
	VecPosition me = worldModel->getMyPositionGroundTruth();
	me.setZ(0);
	if (me.getX() < maxP.getX()) {
		ret = maxP.getX() - start.getX() + maxP.getDistanceTo(me);
	} else {
		ret = start.getDistanceTo(me);
	}
	if (maxP.getX() <= me.getX())
		maxP = me;
//	cout<<fabs(me.getX() - start.getX())<<" "<<fabs(me.getY() - start.getY())<<endl;
	if (fabs(me.getX() - start.getX()) >= 11
			|| fabs(me.getY() - start.getY()) >= 11)
		return -20;
	return ret;
}
double OptimizationBehaviorWalkForward::fitnessDirect(VecPosition start) {
	double ret = 0;
	VecPosition me = worldModel->getMyPositionGroundTruth();
	me.setZ(0);

	ret = me.getDistanceTo(start);
	return ret;
}

double OptimizationBehaviorWalkForward::fitnessLat(VecPosition start) {
	double ret = 0;
	VecPosition me = worldModel->getMyPositionGroundTruth();
	me.setZ(0);
	ret = abs(me.getY() - start.getY());
	return ret;
}

double OptimizationBehaviorWalkForward::fitnessSpiral(VecPosition start) {
	VecPosition me = worldModel->getMyPositionGroundTruth();
	me.setZ(0);

	if (me.getY() == start.getY() && abs(me.getX() - start.getX()) >= 0.7) {
		return 1;
	} else
		return 0;
}
double OptimizationBehaviorWalkForward::fitnessStop(VecPosition start) {
	VecPosition me = worldModel->getMyPositionGroundTruth();
	if (me.getZ() <= 0.45)
		return 1;
	else
		return 0;
}
double OptimizationBehaviorWalkForward::fitnessDiagonal(VecPosition start) {
	double ret = 0;
	VecPosition me = worldModel->getMyPositionGroundTruth();
	me.setZ(0);

	ret = me.getDistanceTo(start);
	double deltay = fabs(me.getY() - start.getY());
	double deltax = fabs((start.getX() - me.getX()));
	if (deltax == 0 || deltay == 0 || fabs(deltay / deltax - 1) > 0.55)
		ret *= 0.5;
	return ret;
}
double OptimizationBehaviorWalkForward::fitnessTurn(double startAngle,
		VecPosition start, int dir) {
	double ang = worldModel->getMyAngDegGroundTruth();
//	cout<<ang<<endl;
	if (ang < 0)
		ang += 360;
	if (startAngle < 0)
		startAngle += 360;

	double diff = 0;

	if ((ang <= 180 && startAngle <= 180) || (ang > 180 && startAngle > 180))
		diff = fabs(ang - startAngle);
	else
		diff = fabs(min(ang, startAngle) + 365 - max(ang, startAngle));

	if (fabs(diff - 180) < 5.0 && !worldModel->isFallen() && !hasFallen)
		return -0.3;


	return 0;
}
SkillType OptimizationBehaviorWalkForward::weave(double moveStartTime,
		double timePerWeave) {
	double currentTime = worldModel->getTime();
	VecPosition me = worldModel->getMyPositionGroundTruth();

	currentTime -= startTime;
	int t = currentTime - moveStartTime;
	if (t - flipTime >= timePerWeave) {
		flip = !flip;
		flipTime = t;
	}
	if (flip)
		return goToTarget(VecPosition(me.getX() + 1, me.getY() - 1, me.getZ()));
	else
		return goToTarget(VecPosition(me.getX() + 1, me.getY() + 1, me.getZ()));
}
SkillType OptimizationBehaviorWalkForward::randomMove(double moveStartTime,
		double timePerMove) {
	double currentTime = worldModel->getTime();
	VecPosition me = worldModel->getMyPositionGroundTruth();

	currentTime -= startTime;
	double t = currentTime - moveStartTime;
	static VecPosition curTar = VecPosition(-me.getX(), -me.getY(), me.getZ());

	if (t - flipTime >= timePerMove) {
		flipTime = t;
		VecPosition x = VecPosition(((rand() % 30) - 15), ((rand() % 20) - 10),
				me.getZ());
		if ((x.getX() >= 0 && curTar.getX() >= 0)
				|| (x.getX() <= 0 && curTar.getX() <= 0))
			x.setX(x.getX() * -1);
		if ((x.getY() >= 0 && curTar.getY() >= 0)
				|| (x.getY() <= 0 && curTar.getY() <= 0))
			x.setY(x.getY() * -1);
		curTar = x;
	}
	return goToTarget(curTar);
}
SkillType OptimizationBehaviorWalkForward::performMoves() {
	double currentTime = worldModel->getTime();
	currentTime -= startTime;
	VecPosition me = worldModel->getMyPositionGroundTruth();
	if (currentTime <= 15.0) {
		times[0] = 15.0;
		moveTypes[0] = 0;
		if (!targetSet[0]) {
			targets[0] = VecPosition(me.getX() + 20, me.getY(), me.getZ());
			targetSet[0] = 1;
			orientation[0] = 0;
		}
		return walkFront(1.0);

	} else if (currentTime <= 16.0) {
		times[1] = 16.0;
		moveTypes[1] = 5;
		return SKILL_STAND;
	} else if (currentTime <= 22.0) {
		times[2] = 22.0;
		moveTypes[2] = 0;
		if (!targetSet[2]) {
			targets[2] = VecPosition(me.getX() - 20, me.getY(), me.getZ());
			targetSet[2] = 1;
			orientation[2] = 175;
		}
		return walkBackwardsTurn(1.0);
	} else if (currentTime <= 23.0) {
		times[3] = 23.0;
		moveTypes[3] = 5;
		return SKILL_STAND;
	} else if (currentTime <= 27.0) {
		times[4] = 27.0;
		moveTypes[4] = 0;
		if (!targetSet[4]) {
			targets[4] = VecPosition(me.getX() + 20, me.getY(), me.getZ());
			targetSet[4] = 1;
			orientation[4] = 175;
		}
		return walkBackwards();
	} else if (currentTime <= 28.0) {
		times[5] = 28.0;
		moveTypes[5] = 5;
		return SKILL_STAND;
	} else if (currentTime <= 30.0) {
		times[6] = 30.0;
		moveTypes[6] = 4;
		if (!targetSet[6]) {
			targets[6] = VecPosition(me.getX(), me.getY() + 20, me.getZ());
			targetSet[6] = 1;
			orientation[6] = 175;
		}
		return walkRightLat();
	} else if (currentTime <= 40.0) {
		times[7] = 40.0;
		moveTypes[7] = 0;
		if (!targetSet[7]) {
			targets[7] = VecPosition(me.getX(), me.getY() + 20, me.getZ());
			targetSet[7] = 1;
			orientation[7] = 88;
		}
		return walkRight(1.0);
	} else if (currentTime <= 41.0) {
		times[8] = 41.0;
		moveTypes[8] = 5;
		return SKILL_STAND;
	} else if (currentTime <= 45.0) {
		times[9] = 45.0;
		moveTypes[9] = 4;
		if (!targetSet[9]) {
			targets[9] = VecPosition(me.getX(), me.getY() + 20, me.getZ());
			targetSet[9] = 1;
			orientation[9] = 88;
		}
		return walkLeftLat();
	} else if (currentTime <= 50.0) {
		times[10] = 50.0;
		moveTypes[10] = 0;
		if (!targetSet[0]) {
			targets[10] = VecPosition(me.getX() - 20, me.getY(), me.getZ());
			targetSet[10] = 1;
			orientation[10] = 175;
		}
		return walkBackwardsTurn(20.0);
	} else if (currentTime <= 51.0) {
		times[11] = 51.0;
		moveTypes[11] = 5;
		return SKILL_STAND;
	} else if (currentTime <= 66.0) {
		times[12] = 66.0;
		moveTypes[12] = 1;
		return walkInACircle();
	} else if (currentTime <= 67.0) {
		times[13] = 67.0;
		moveTypes[13] = 5;
		return SKILL_STAND;
	} else if (currentTime <= 72.0) {
		times[14] = 72.0;
		moveTypes[14] = 1;
		return walkInACircle();
	} else if (currentTime <= 73.0) {
		times[15] = 73.0;
		moveTypes[15] = 5;
		return SKILL_STAND;
	} else if (currentTime <= 78.0) {
		times[16] = 78.0;
		moveTypes[16] = 0;
		if (!targetSet[16]) {
			targets[16] = VecPosition(me.getX() - 20, me.getY(), me.getZ());
			targetSet[16] = 1;
		}
		return walkBackwardsTurn(1.0);
	} else if (currentTime <= 88.0) {
		times[17] = 88.0;
		moveTypes[17] = 2;
		if (!targetSet[17]) {
			targets[17] = VecPosition(me.getX() + 10, me.getY() - 10,
					me.getZ());
			targetSet[17] = 1;
			orientation[17] = 45;
		}
		return walk45up(1.0);
	} else if (currentTime <= 98.0) {
		times[18] = 98.0;
		moveTypes[18] = 2;
		if (!targetSet[18]) {
			targets[18] = VecPosition(me.getX() - 20, me.getY() + 20,
					me.getZ());
			targetSet[18] = 1;
			orientation[18] = 140;
		}
		return walk45down(1.0);
	} else if (currentTime <= 99.0) {
		times[19] = 88.0;
		moveTypes[19] = 5;
		return SKILL_STAND;
	} else if (currentTime <= 109.0) {
		times[20] = 109.0;
		moveTypes[20] = 0;
		if (!targetSet[20]) {
			targets[20] = VecPosition(me.getX() + 20, me.getY(), me.getZ());
			targetSet[20] = 1;
		}
		return weave(99.0, 1.0);
	} else if (currentTime <= 110.0) {
		times[21] = 110.0;
		moveTypes[21] = 0;
		flipTime = 0;
		return walkRight(1.0);
	} else if (currentTime <= 120.0) {
		times[22] = 120.0;
		moveTypes[22] = 0;
		if (!targetSet[22]) {
			targets[22] = VecPosition(me.getX() + 20, me.getY(), me.getZ());
			targetSet[22] = 1;
		}
		return weave(110.0, 3.0);
	} else if (currentTime <= 121.0) {
		times[23] = 121.0;
		moveTypes[23] = 0;
		flipTime = 0;
		return walkBackwardsTurn(1.0);
	} else if (currentTime <= 131.0) {
		times[24] = 131.0;
		moveTypes[24] = 0;
		return randomMove(121.0, 1.5);
	} else if (currentTime <= 132.0) {
		times[25] = 132.0;
		moveTypes[25] = 0;
		flipTime = 0;
		return walkBackwardsTurn(1.0);
	} else if (currentTime <= 142.0) {
		times[26] = 142.0;
		moveTypes[26] = 0;
		return randomMove(132.0, 4.0);
	} else if (currentTime <= 155.0) {
		times[27] = 155.0;
		moveTypes[27] = 3;
		return turnCW();
	}
	//else if (currentTime <= 154.0) {
	//	times[28] = 154.0;
	//	moveTypes[28] = 5;
	//	return walkFront(1.0);
	//} else if (currentTime <= 160.0) {
	//	times[29] = 160.0;
	//	moveTypes[29] = 3;
	//	return turnCW();
	//}

	return SKILL_STAND;
}

SkillType OptimizationBehaviorWalkForward::selectSkill() {
	double currentTime = worldModel->getTime();
	if (currentTime - startTime < INIT_WAIT || startTime < 0 || standing) {
		return SKILL_STAND;
	}

	return performMoves();
//	if (run == 0)
//		return walkFront(10);
//	else if (run == 1) {
//		return walkBackwards();
//	} else if (run == 2) {
//		return walkBackwardsTurn(10);
//	} else if (run == 3) {
//		return walkLeftLat();
//	} else if (run == 4) {
//		return walkRightLat();
//	} else if (run == 5) {
//		return walkLeft(10);
//	} else {
//		return walkRight(10);
//	}
//
//	if (run != 7 && run != 8) {
//		for (int j = movements[run].size() - 1; j >= 0; j--) {
//			if (currentTime - targetStartTime <= movements[run][j].second
//					&& currentTime - targetStartTime
//							> ((j == 0) ? -1 : movements[run][j - 1].second)) {
//				return (movements[run][j].first.stop) ?
//						SKILL_STAND :
//						goToTargetRelative(movements[run][j].first.x,
//								movements[run][j].first.angle);
//			}
//		}
//	}
//	if (run == 7) {
//		if (currentTime - randomIsSetTime >= 0.9) {
//			randomTarget = VecPosition(randomTarget.getX() * -1,
//					randomTarget.getY() * -1, 0);
//			randomIsSetTime = currentTime;
//		}
//		return goToTarget(randomTarget);
//
//	} else {
//		VecPosition center = VecPosition(-HALF_FIELD_X / 2.0, 0, 0);
//		double circleRadius = 7.0;
//		double rotateRate = 3;
//		VecPosition localCenter = worldModel->g2l(center);
//		SIM::AngDeg localCenterAngle = atan2Deg(localCenter.getY(),
//				localCenter.getX());
//
//		// Our desired target position on the circle
//		// Compute target based on uniform number, rotate rate, and time
//		VecPosition target = center
//				+ VecPosition(circleRadius, 0, 0).rotateAboutZ(
//						360.0 / (NUM_AGENTS - 1) * (worldModel->getUNum())
//								+ worldModel->getTime() * rotateRate);
//
//		// Adjust target to not be too close to teammates or the ball
//		target = collisionAvoidance(true /*teammate*/, false/*opponent*/,
//				true/*ball*/, 1/*proximity thresh*/, .5/*collision thresh*/,
//				target, true/*keepDistance*/);
//
//		if (me.getDistanceTo(target) < .25 && abs(localCenterAngle) <= 10) {
//			// Close enough to desired position and orientation so just stand
//			return SKILL_STAND;
//		} else if (me.getDistanceTo(target) < .5) {
//			// Close to desired position so start turning to face center
//			return goToTargetRelative(worldModel->g2l(target), localCenterAngle);
//		} else {
//			// Move toward target location
//			return goToTarget(target);
//		}
//	}

}

void OptimizationBehaviorWalkForward::updateFitness() {
	static bool written = false;

	if (run == 3) {
		if (!written) {
			double fitness = totalWalkDist / (double) run;
			fstream file;
			file.open(outputFile.c_str(), ios::out);
			file << fitness << endl;
			file.close();
			written = true;
		}
		return;
	}

	if (startTime < 0) {
		init();
		return;
	}

	double currentTime = worldModel->getTime();
	if (currentTime - startTime < INIT_WAIT) {
		return;
	}

	if (!beamChecked) {
		static bool failedLastBeamCheck = false;
		if (!checkBeam()) {
			// Beam failed so reinitialize everything
			if (failedLastBeamCheck) {
				// Probably something bad happened
				//if we failed the beam twice in a row (perhaps the agent can't stand) so give a bad score and
				// move on
				totalWalkDist -= 100;
				run++;
			}
			failedLastBeamCheck = true;
			init();
			return;
		} else {
			failedLastBeamCheck = false;
			// Set playmode to PlayOn to start run and move ball out of the way
			string msg = "(playMode PlayOn) (ball (pos 0 -9 0) (vel 0 0 0))";
			setMonMessage(msg);
		}
	}
	VecPosition me = worldModel->getMyPositionGroundTruth();
	if (worldModel->isFallen() && !hasFallen) {
		hasFallen = true;
		falls++;
	}
	if (worldModel->isFallen() == false && hasFallen) {
		hasFallen = false;
	}
//	static double costs[4];
//	static double timeOfStart[4];
//	for (int phase = 0; phase < 4; phase++) {
//
//		if (currentTime - startTime <= times[phase]) {
//			if (!startSet[phase]) {
//				startt[phase] = me;
//				angle = worldModel->getMyAngDegGroundTruth();
//				startSet[phase] = 1;
//				timeOfStart[phase] = worldModel->getTime();
//				startCalc = false;
//			}
//			if (moveTypes[phase] == 0) {
//				DIS[phase] = fitnessDirect(startt[phase]);
//				if (DIS[phase] >= 1.0 && !costSet[phase]) {
//					costs[phase] = worldModel->getTime() - timeOfStart[phase];
//					costSet[phase] = true;
//				}
//			} else if (moveTypes[phase] == 3) {
//				DIS[phase] += fitnessTurn(angle, startt[phase]);
//				if (fabs(worldModel->getMyAngDegGroundTruth() - angle) <= 5
//						&& !costSet[phase]
//						&& worldModel->getTime() > timeOfStart[phase] + 1) {
//
//					costs[phase] = worldModel->getTime() - timeOfStart[phase];
//					costSet[phase] = true;
//				}
//			}
//
//			break;
//		}
//	}
	if (currentTime - targetStartTime > 155.0) {
//		cout<<falls<<endl;
	//	for (int phase = 0; phase < 28; phase++)
		//	cout << moveTypes[phase] << " " << phase << " " << DIS[phase]
			//		<< endl;
//		cout << costs[0] << " " << costs[1] << " " << costs[2] << " "<<costs[3]<< endl;
		double fit = 0;
		fit += (falls * COST_OF_FALL * -1);
		bool x = false;
		for (int phase = 0; phase < 28; phase++) {

			if (moveTypes[phase] == 5) {
//					cout<<5<<" "<<(DIS[phase] > 0)* -3<<endl;
//				cout << (DIS[phase] > 0) * -3 << endl;
				fit += (DIS[phase] > 0) * -5;
			} else if (moveTypes[phase] != 3 && moveTypes[phase] != 6) {
				if (endsAngles[phase] < 0)
					endsAngles[phase] *= -1;
				if (DIS[phase] <= 0.1)
					x = true;
//					cout<<moveTypes[phase]<<" "<<DIS[phase]<<endl;
				if (targetSet[phase] == true) {
					if (startt[phase].getDistanceTo(targets[phase])
							< ends[phase].getDistanceTo(targets[phase])) {
//						cout << startt[phase] << " " << ends[phase] << " "
//								<< targets[phase] << endl;
//						cout << phase << " " << -12 << endl;
						fit -= 12;
					} else if (orientation[phase] < 700
							&& fabs(endsAngles[phase] - orientation[phase])
									> 15.0) {
						cout << phase << " " << endsAngles[phase] << " "
								<< orientation[phase] << " " << -12 << endl;
						fit -= 12;
					} else {
//						cout << phase << " " << DIS[phase] << endl;
						fit += DIS[phase];
					}

				} else {
//					cout << phase << " " << DIS[phase] << endl;
					fit += DIS[phase];
				}

			} else {
//					cout<<"turn"<<" "<<DIS[phase]<<endl;
//				cout << 1.5 * (DIS[phase] > 0) - 1.5 * (DIS[phase] == 0)
//						<< endl;
				fit -= DIS[phase];
			}
		}
		fit -= x * 5;
		totalWalkDist += fit;
		run++;
		cout << "Run " << run << " : distance walked " << fit << endl;
		init();
	}
//	DIS[phase] += fitnessTurn(angle, startt[phase], 0);
//	ends[phase] = me;
//	endsAngles[phase] = worldModel->getMyAngDegGroundTruth();
//	cout << "fit " << DIS[phase] << endl;
	for (int phase = 0; phase < 28; phase++) {

		if (currentTime - startTime <= times[phase]) {

			if (!startSet[phase]) {
				startt[phase] = me;
				angle = worldModel->getMyAngDegGroundTruth();
				startSet[phase] = 1;
				startCalc = false;
			}

			if (moveTypes[phase] == 0) {
				DIS[phase] = fitnessDirect(startt[phase]);
			} else if (moveTypes[phase] == 1) {
				DIS[phase] = fitnessCircle(startt[phase]);
			} else if (moveTypes[phase] == 2) {
				DIS[phase] = fitnessDiagonal(startt[phase]);
			} else if (moveTypes[phase] == 3) {
				DIS[phase] += fitnessTurn(angle, startt[phase], 0);
			} else if (moveTypes[phase] == 6) {
				DIS[phase] += fitnessTurn(angle, startt[phase], 1);
			} else if (moveTypes[phase] == 4) {
				DIS[phase] = fitnessLat(startt[phase]);
			} else if (moveTypes[phase] == 5)
				DIS[phase] += fitnessStop(startt[phase]);
			ends[phase] = me;
			if((phase == 0 && currentTime - startTime - INIT_WAIT <= 1.9) || (phase != 0 && currentTime - startTime - times[phase-1] <= 1.9))
				{
					endsAngles[phase] = worldModel->getMyAngDegGroundTruth();
//					cout<<((phase == 0)? currentTime - startTime - INIT_WAIT : currentTime - startTime - times[phase-1])<<" angle: "<<endsAngles[phase]<<endl;
				}
			break;
		}
	}

//	int idx = run;
//	if (run < 5) {
//		for (int j = movements[idx].size() - 1; j >= 0; j--) {
//			if (currentTime - targetStartTime <= movements[idx][j].second
//					&& currentTime - targetStartTime
//							> ((j == 0) ? -1 : movements[idx][j - 1].second)) {
//				if (!startSet[j]) {
//					startt = me;
//					angle = worldModel->getMyAngDegGroundTruth();
//					startSet[j] = 1;
//					startCalc = false;
//				}
//				if (movementsTypes[idx][j] == 0) {
//					DIS[j] = fitnessDirect(startt);
//				} else if (movementsTypes[idx][j] == 1) {
//					DIS[j] = fitnessCircle(startt);
//				} else if (movementsTypes[idx][j] == 2) {
//					DIS[j] = fitnessDiagonal(startt);
//				} else if (movementsTypes[idx][j] == 3) {
//					DIS[j] += fitnessTurn(angle, startt);
//				} else if (movementsTypes[idx][j] == 4)
//					DIS[j] = fitnessLat(startt);
//				else if (movementsTypes[idx][j] == 5)
//					DIS[j] += fitnessStop(startt);
//			}
//		}
////	}
//	else if (run == 4) {
//				if (falls == 0)
//					fit += 1.5 * (DIS[0] > 0) + 1.5 * (DIS[1] > 0);
//				else
//					fit -= 3;
//			} else if (run == 5) {
//				double dist = fitnessDiagonal(startt);
//				if (dist <= 0.1)
//					fit -= 2;
//				fit += dist;
//			} else {
//				double dist = fitnessDirect(startt);
//				if (dist <= 0.1)
//					fit -= 2;
//				fit += dist * 0.8;
//			}

}

void OptimizationBehaviorWalkForward::beam(double& beamX, double& beamY,
		double& beamAngle) {
	if (run != 8) {
		beamX = -1;
		beamY = 0;
		beamAngle = 0;
	} else {

		VecPosition center = VecPosition(-HALF_FIELD_X / 2.0, 0, 0);
		double circleRadius = 7.0;
		double rotateRate = 3;
		VecPosition localCenter = worldModel->g2l(center);
		SIM::AngDeg localCenterAngle = atan2Deg(localCenter.getY(),
				localCenter.getX());

		// Our desired target position on the circle
		// Compute target based on uniform number, rotate rate, and time
		VecPosition target = center
				+ VecPosition(circleRadius, 0, 0).rotateAboutZ(
						360.0 / (NUM_AGENTS - 1) * (worldModel->getUNum())
								+ worldModel->getTime() * rotateRate);

		// Adjust target to not be too close to teammates or the ball
		target = collisionAvoidance(true /*teammate*/, false/*opponent*/,
				true/*ball*/, 1/*proximity thresh*/, .5/*collision thresh*/,
				target, true/*keepDistance*/);

		beamX = -2;
		beamY = 0;
		beamAngle = 45;
	}
}

bool OptimizationBehaviorWalkForward::checkBeam() {
	LOG_STR("Checking whether beam was successful");
	VecPosition meTruth = worldModel->getMyPositionGroundTruth();
	meTruth.setZ(0);
	double beamX, beamY, beamAngle;
	beam(beamX, beamY, beamAngle);
	VecPosition meDesired = VecPosition(beamX, beamY, 0);
	double distance = meTruth.getDistanceTo(meDesired);
	double angleOffset = abs(worldModel->getMyAngDegGroundTruth() - beamAngle);
	if (distance > 0.05 || angleOffset > 5) {
		LOG_STR("Problem with the beam!");
		LOG(distance);
		LOG(meTruth);
		return false;
	}
	beamChecked = true;
	return true;
}
/*
 *
 *
 * STAND OPTIMIZATION AGENT
 *
 *
 *
 */

OptimizationBehaviorStand::OptimizationBehaviorStand(const std::string teamName,
		int uNum, const map<string, string>& namedParams_, const string& rsg_,
		const string& outputFile_) :
		NaoBehavior(teamName, uNum, namedParams_, rsg_), outputFile(
				outputFile_), stand(0), INIT_WAIT_TIME(3.0) {
	totalFitness = 0.0;
	initStand();
}

void OptimizationBehaviorStand::beam(double& beamX, double& beamY,
		double& beamAngle) {
	beamX = -5.0;
	beamY = -5.0;
	beamAngle = 0.0;
}

SkillType OptimizationBehaviorStand::selectSkill() {
	double time = worldModel->getTime();
	if (timeStart < 0) {
		initStand();
		return SKILL_STAND;
	}

// Wait a bit before attempting kick
	if (time - timeStart <= INIT_WAIT_TIME) {
		return SKILL_STAND;
	}

	if (time - timeStart < 5)
		return SKILL_DIVE_LEFT;

	return SKILL_STAND;

}

void OptimizationBehaviorStand::updateFitness() {

	if (stand == 10) {
		writeFitnessToOutputFile(totalFitness / (double(stand)));
		return;
	}

	double time = worldModel->getTime();
	if (time - timeStart <= INIT_WAIT_TIME) {
		return;
	}

	if (!beamChecked) {
		beamChecked = true;

		// Set playmode to PlayOn to start run and move ball out of the way
		string msg = "(playMode PlayOn)";
		setMonMessage(msg);
	}

	if (worldModel->getMyPositionGroundTruth().getZ() < 0.47)
		stand_up_time = 0.0;

	if (worldModel->getMyPositionGroundTruth().getZ() >= 0.48
			&& (time - (timeStart + INIT_WAIT_TIME)) > 2) {
		if (stand_up_time == 0)
			stand_up_time = time;

		else if (time - stand_up_time > 2) {
			cout << "Stand " << stand << " Fitness = "
					<< (time - (timeStart + INIT_WAIT_TIME)) << endl;
			totalFitness += (time - timeStart);
			stand++;
			initStand();
			return;
		}
	}

	VecPosition meTruth = worldModel->getMyPositionGroundTruth();
	meTruth.setZ(0);

	if (time - (timeStart + INIT_WAIT_TIME) > 15) {

		totalFitness += 100;
		stand++;
		cout << "Stand " << stand << " Fitness = "
				<< (time - (timeStart + INIT_WAIT_TIME)) << endl;
		initStand();
		return;

	}
}

void OptimizationBehaviorStand::initStand() {

	beamChecked = false;
	timeStart = worldModel->getTime();
	initialized = false;
	initBeamed = false;
	fallen = false;
	stand_up_time = 0.0;
	resetSkills();

// Beam agent and ball
	double beamX, beamY, beamAngle;
	beam(beamX, beamY, beamAngle);
	VecPosition beamPos = VecPosition(beamX, beamY, 0);
	string msg = "(playMode BeforeKickOff)";
	setMonMessage(msg);
}

void OptimizationBehaviorStand::writeFitnessToOutputFile(double fitness) {
	static bool written = false;
	if (!written) {
		LOG(fitness);
		LOG(stand);
		fstream file;
		file.open(outputFile.c_str(), ios::out);
		file << fitness << endl;
		file.close();
		written = true;
		//string msg = "(killsim)";
		//setMonMessage(msg);
	}
}

bool OptimizationBehaviorStand::checkBeam() {
	LOG_STR("Checking whether beam was successful");
	VecPosition meTruth = worldModel->getMyPositionGroundTruth();
	meTruth.setZ(0);
	double beamX, beamY, beamAngle;
	beam(beamX, beamY, beamAngle);
	VecPosition meDesired = VecPosition(beamX, beamY, 0);
	double distance = meTruth.getDistanceTo(meDesired);
	double angleOffset = abs(worldModel->getMyAngDegGroundTruth() - beamAngle);
	if (distance > 0.05 || angleOffset > 5) {
		LOG_STR("Problem with the beam!");
		LOG(distance);
		LOG(meTruth);
		return false;
	}
	beamChecked = true;
	return true;
}
