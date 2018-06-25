class Taches {

 //attribut
  private :
    int DayDistrib;
    int HeureDistrib;
    int MinuteDistrib;
    bool ActiveDistrib;
    int QuantiteDistrib;

  public :
  //Methode

  void setup(){
    DayDistrib=0;
    HeureDistrib=0;
    MinuteDistrib=0;
    ActiveDistrib=false;
    QuantiteDistrib=0;
  }

  int get_date(){ //retourner le jour de distribution
    return DayDistrib;
  }

  int get_heure(){ //retourner l'heure de distribution 
    return HeureDistrib;
  }

  int get_minute(){ // retourner les minute de la distribution  
    return MinuteDistrib;
  }

  int get_nbDose(){ // retourner la quantité de dose
    return QuantiteDistrib;
  }

  bool get_status(){ // retourner le statut de la tache (ON/OFF)
    return ActiveDistrib;
  }
  char get_all(){
    char tache;
    tache=DayDistrib;
    
    //printf("%s/%s:%s/%s/%s",DayDistrib,HeureDistrib,MinuteDistrib,QuantiteDistrib,ActiveDistrib);
    //Serial.print(DayDistrib + "/" + HeureDistrib + ":"+ MinuteDistrib+"/"+QuantiteDistrib+"/"+ActiveDistrib);
    return tache;
  }
  

  // Methode update

  void set_date(int D){ //Mettre a jour la date
    DayDistrib=D;
  }

  void set_heure(int H){ //Mettre à jour l'heure
    HeureDistrib=H;
  }

  void set_minute(int m){ //mettre a jour les minute
    MinuteDistrib=m;
  }

  void set_nbDose(int nb){ // mettre a jour le nombre de dose
    QuantiteDistrib=nb;
  }

  void set_status(bool state){  // mettre a jour le statut
    ActiveDistrib=state;
  }

 void set_global(int D, int H, int m, int nb, bool state){
    DayDistrib=D;
    HeureDistrib=H;
    MinuteDistrib=m;
    QuantiteDistrib=nb;
    ActiveDistrib=state;
 }

  
};

