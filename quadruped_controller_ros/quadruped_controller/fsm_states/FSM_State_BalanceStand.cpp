/*=========================== Balance Stand ===========================*/
/**
 * FSM State that forces all legs to be on the ground and uses the QP
 * Balance controller for instantaneous balance control.
 */

#include "FSM_State_BalanceStand.h"
#include "WBC_Ctrl/LocomotionCtrl/LocomotionCtrl.hpp"

/**
 * Constructor for the FSM State that passes in state specific info to
 * the generic FSM State constructor.
 *
 * @param _controlFSMData holds all of the relevant control data
 */
template <typename T>
FSM_State_BalanceStand<T>::FSM_State_BalanceStand(
    ControlFSMData<T>* _controlFSMData)
    : FSM_State<T>(_controlFSMData, FSM_StateName::BALANCE_STAND,"BALANCE_STAND") {
  // Set the pre controls safety checks
  this->turnOnAllSafetyChecks();
  // Turn off Foot pos command since it is set in WBC as operational task
  this->checkPDesFoot = false;


  // Initialize GRF to 0s
  this->footFeedForwardForces = Mat34<T>::Zero();

  _wbc_ctrl = new LocomotionCtrl<T>(_controlFSMData->_quadruped->buildModel());
  _wbc_data = new LocomotionCtrlData<T>();

  _wbc_ctrl->setFloatingBaseWeight(1000.);
}

template <typename T>
void FSM_State_BalanceStand<T>::onEnter() {
  
	printf("Entered BalanceStand\n");

  // Always set the gait to be standing in this state
  this->_data->_gaitScheduler->gaitData._nextGait = GaitType::STAND;
  
  _ini_body_pos = (this->_data->_stateEstimator->getResult()).position;

  if(_ini_body_pos[2] < 0.2) {
    _ini_body_pos[2] = 0.3;
  }

  last_height_command = _ini_body_pos[2];

  _ini_body_ori_rpy = (this->_data->_stateEstimator->getResult()).rpy;
  _body_weight = this->_data->_quadruped->_bodyMass * 9.81;
}

/**
 * Calls the functions to be executed on each control loop iteration.
 */
template <typename T>
void FSM_State_BalanceStand<T>::run() {
  Vec4<T> contactState;
  contactState<< 0.5, 0.5, 0.5, 0.5;
  this->_data->_stateEstimator->setContactPhase(contactState);
  BalanceStandStep();
}


/**
 * Cleans up the state information on exiting the state.
 */
template <typename T>
void FSM_State_BalanceStand<T>::onExit() {
  _iter = 0;
}

/**
 * Calculate the commands for the leg controllers for each of the feet.
 */
template <typename T>
void FSM_State_BalanceStand<T>::BalanceStandStep() {

  _wbc_data->pBody_des = _ini_body_pos;
  _wbc_data->vBody_des.setZero();
  _wbc_data->aBody_des.setZero();

  _wbc_data->pBody_RPY_des = _ini_body_ori_rpy;
  
    // Orientation
    _wbc_data->pBody_RPY_des[0] = 
        0.6* this->_data->_desiredStateCommand->leftAnalogStick[0];
        _wbc_data->pBody_RPY_des[1] = 
        0.6*this->_data->_desiredStateCommand->rightAnalogStick[0];
    _wbc_data->pBody_RPY_des[2] -= 
        this->_data->_desiredStateCommand->rightAnalogStick[1];

    // Height
    _wbc_data->pBody_des[2] += 
        0.12 * this->_data->_desiredStateCommand->rightAnalogStick[0];
  
  _wbc_data->vBody_Ori_des.setZero();

  for(size_t i(0); i<4; ++i){
    _wbc_data->pFoot_des[i].setZero();
    _wbc_data->vFoot_des[i].setZero();
    _wbc_data->aFoot_des[i].setZero();
    _wbc_data->Fr_des[i].setZero();
    _wbc_data->Fr_des[i][2] = _body_weight/4.;
    _wbc_data->contact_state[i] = true;
  }
  
  if(this->_data->_desiredStateCommand->trigger_pressed) {
    _wbc_data->pBody_des[2] = 0.05;

    if(last_height_command - _wbc_data->pBody_des[2] > 0.001) {
      _wbc_data->pBody_des[2] = last_height_command - 0.001;
    }
  }
  last_height_command = _wbc_data->pBody_des[2];

  _wbc_ctrl->run(_wbc_data, *this->_data);
}

// template class FSM_State_BalanceStand<double>;
template class FSM_State_BalanceStand<float>;
