#ifndef __RTS2_DAEMON__
#define __RTS2_DAEMON__

#include "rts2block.h"
#include "rts2logstream.h"
#include "rts2value.h"
#include "rts2valuelist.h"

#include <vector>

/**
 * Abstract class for centrald and all devices.
 *
 * This class contains functions which are common to components with one listening socket.
 */
class Rts2Daemon:public Rts2Block
{
private:
  // 0 - don't deamonize, 1 - do deamonize, 2 - is already deamonized, 3 - deamonized & centrald is running, don't print to stdout
  enum
  { DONT_DEAMONIZE, DO_DEAMONIZE, IS_DEAMONIZED, CENTRALD_OK } daemonize;
  int listen_sock;
  void addConnectionSock (int in_sock);
  int lockf;

  Rts2CondValueVector values;
  // values which do not change, they are send only once at connection
  // initialization
  Rts2ValueVector constValues;

  Rts2ValueTime *info_time;

  time_t idleInfoInterval;
  time_t nextIdleInfo;

  /**
   * Adds value to list of values supported by daemon.
   *
   * \param value Rts2Value which will be added.
   */
  void addValue (Rts2Value * value, int queCondition = 0);
  Rts2CondValue *getValue (const char *v_name);
protected:
  int checkLockFile (const char *lock_fname);
  int doDeamonize ();
  int lockFile ();
  virtual void addSelectSocks (fd_set * read_set);
  virtual void selectSuccess (fd_set * read_set);

  Rts2ValueQueVector queValues;

  /**
   * Create new value, and return pointer to it.
   *
   * \param value          Rts2Value which will be created
   * \param in_val_name    value name
   * \param in_description value description
   * \param writeToFits    when true, value will be writen to FITS
   */
    template < typename T > void createValue (T * &val, char *in_val_name,
					      std::string in_description,
					      bool writeToFits =
					      true, int queCondition = 0)
  {
    val = new T (in_val_name, in_description, writeToFits);
    addValue (val, queCondition);
  }
  /**
   * Create new constant value, and return pointer to it.
   *
   * \param value          Rts2Value which will be created
   * \param in_val_name    value name
   * \param in_description value description
   * \param writeToFits    when true, value will be writen to FITS
   */
  template < typename T > void createConstValue (T * &val, char *in_val_name,
						 std::string in_description,
						 bool writeToFits = true)
  {
    val = new T (in_val_name, in_description, writeToFits);
    addConstValue (val);
  }
  void addConstValue (Rts2Value * value);
  void addConstValue (char *in_name, const char *in_desc, char *in_value);
  void addConstValue (char *in_name, const char *in_desc, double in_value);
  void addConstValue (char *in_name, const char *in_desc, int in_value);
  void addConstValue (char *in_name, char *in_value);
  void addConstValue (char *in_name, double in_value);
  void addConstValue (char *in_name, int in_value);

  /** 
   * Set value. That one should be owerwrited in descendants.
   * 
   * \param  old_value	old value (pointer), can be directly
   * 			compared by pointer stored in object
   * \param  new_value	new value
   * \return 0 when we can set value, -2 on error, 
   */
  virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
  int setValue (Rts2Value * old_value, char op, Rts2Value * new_value);

  /**
   * Returns whenever value change with old_value needs to be qued or
   * not.
   * \param old_value Rts2CondValue object describing the old_value
   */
  virtual bool queValueChange (Rts2CondValue * old_value) = 0;
public:
  Rts2Daemon (int in_argc, char **in_argv);
  virtual ~ Rts2Daemon (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int initValues ();
  void initDaemon ();
  virtual int run ();
  virtual int idle ();

  void setIdleInfoInterval (time_t interval)
  {
    idleInfoInterval = interval;
  }

  virtual void forkedInstance ();
  virtual void sendMessage (messageType_t in_messageType,
			    const char *in_messageString);
  virtual void centraldConnRunning ();
  virtual void centraldConnBroken ();

  virtual int baseInfo ();
  int baseInfo (Rts2Conn * conn);
  int sendBaseInfo (Rts2Conn * conn);

  virtual int info ();
  int info (Rts2Conn * conn);
  int infoAll ();
  int sendInfo (Rts2Conn * conn);
  int sendMetaInfo (Rts2Conn * conn);

  virtual int setValue (Rts2Conn * conn);
};

#endif /* ! __RTS2_DAEMON__ */
