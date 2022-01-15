function seznam_razlik(seznam){
    const razlika_seznam=[];
    razlika_seznam.push(seznam[0]);
    var sestej=seznam[0];
    for(var i=1;i<seznam.length;i++){
        var razlika=seznam[i]+sestej;
        sestej=razlika;
        razlika_seznam.push(razlika);
    }
    return razlika_seznam;
}
function absolutna_vrednost(st){
    if(st.substring(0,1)=="1"){
        var prvo=st.substring(1,9);
        var p=parseInt(prvo,2);
        return -p
    }
    else if(st.substring(0,1)=="0"){
        var prvo=st.substring(1,9);
        var p=parseInt(prvo,2);
        return p
    }
}
function RazsiriPodatke(bajti) {
    try {
        lista = ""
        var odstrig = 8 - bajti[0];
        if(bajti[0] == 0){bajti[0] = 8;}
        for(var n = 1; n < bajti.length; n++){
            for(var k=0; k<8; k++){
                if(n == (bajti.length-1) && bajti[0] == k){
                    break;
                }
                if(bajti[n] & (1<<k)){
                    lista += '1'
                }
                else{
                    lista += '0'
                }
            }
        }
        //console.log(lista, odstrig);
        const l=[];

        let prvo=lista.substring(0,8);
        let z=parseInt(prvo,2);
        l.push(z);
        var velikost=lista.length-3;

        var ll=seznam_razlik(l);
        return ll;
    }catch (error) {
        console.log(error);
        return [0,0,0,0]
    }
}