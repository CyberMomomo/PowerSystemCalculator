#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QIODevice>
#include <QString>

#include <stdio.h>
#include <math.h>
static QJsonObject inputData;
static QJsonArray power_active, power_reactive, resistance, reactance;
static double voltage;
static double delta_s_r[100];
static double delta_s_i[100];
static double sout_r[100];
static double sout_i[100];
static double sa_r[100];
static double sa_i[100];
static double temps_r = 0;
static double temps_i = 0;
static double delta_u[100];
static double d_u[100];
static double u[100];
static double u_r[100];
static double u_i[100];
static double diss[100];
static double diss_acu[100];
static double temp_u = 0;
static bool consider_balance = false;
static double ua_vs_un = 1;

int main(){
    //json文件读取
    QString filePath = "inputData.json";
    QFile file(filePath);
    file.open(QIODevice::ReadOnly);
    QString json = file.readAll();
    QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());    //utf-8编码
    file.close();
    //json内容提取
    inputData = document.object();
    power_active = inputData.value("ActivePower").toArray();       //有功功率P
    power_reactive = inputData.value("ReactivePower").toArray();   //无功功率Q
    resistance = inputData.value("Resistance").toArray();           //电阻R
    reactance = inputData.value("Reactance").toArray();             //电抗X
    voltage = inputData.value("Voltage").toInt();                   //电压等级
    consider_balance = inputData.value("ConsiderBalance").toBool(); //是否考虑平衡分量
    ua_vs_un = inputData.value("UAvsUN").toDouble();

    //前推过程
    char n = 0;
    foreach (QJsonValue tempd, power_active) {
        double temp = 0;
        if(n > 0){
            temp = (pow(power_active[n].toDouble() + sout_r[n-1],2) + pow(power_reactive[n].toDouble() + sout_i[n-1],2)) / pow(voltage,2);
        }else{
            temp = (pow(power_active[n].toDouble(),2) + pow(power_reactive[n].toDouble(),2)) / pow(voltage,2);
        }
        delta_s_r[n] = temp * resistance[n].toDouble();
        delta_s_i[n] = temp * reactance[n].toDouble();
        sa_r[n] = delta_s_r[n] + power_active[n].toDouble();
        sa_i[n] = delta_s_i[n] + power_reactive[n].toDouble();
        sout_r[n] = sa_r[n] + temps_r;
        sout_i[n] = sa_i[n] + temps_i;
        temps_r = sout_r[n];
        temps_i = sout_i[n];
        n++;
    }

    //回代过程
    temp_u = voltage * ua_vs_un;
    for(char i = n-1; i >= 0; i--){
        delta_u[i] = (sout_r[i] * resistance[i].toDouble() + sout_i[i] * reactance[i].toDouble()) / temp_u;
        u_r[i] = temp_u - delta_u[i];
        /*if(i < (n-1)){
            diss[i] = delta_u[i] / temp_u * 100;
            diss_acu[i] = (u[n-1] - u[i]) / voltage * 100;
        }*/
        if (consider_balance) {
            d_u[i] = (sout_r[i] * reactance[i].toDouble() - sout_i[i] * resistance[i].toDouble()) / temp_u;
            u_i[i] = -d_u[i];
        }else {
            u_i[i] = 0;
        }
        u[i] = pow((pow(u_r[i], 2) + pow(u_i[i], 2)), 0.5);
        temp_u = u[i];
    }

    printf_s("========前推==================\n");
    for(char i = 0; i < n; i++){
        printf_s("第%d步：ΔS(%ds) = %f + j%f MVA\n", i+1, i+1, delta_s_r[i], delta_s_i[i]);
        printf_s("        S'(%d) = %f + j%f MVA\n", i+1, sout_r[i], sout_i[i]);
    }

    printf_s("========回代==================\n");
    if(consider_balance) {
        printf_s("========已考虑平衡分量========\n");
    } else {
        printf_s("========未考虑平衡分量========\n");
    }
    for(char i = n-1; i >= 0; i--){
        printf_s("第%d步：ΔU(%d) = %f kV\n", n-i, i+1, delta_u[i]);
        if (consider_balance) {
            printf_s("        dU(%d) = %f kV\n", i+1, d_u[i]);
            printf_s("        U(%d) = %f - j%f kV\n", i+1, u_r[i], d_u[i]);
            printf_s("        U(%d) = %f kV\n", i+1, u[i]);
        } else {
            printf_s("        U(%d) = %f kV\n", i+1, u[i]);
        }
        /*if(i < (n-1)){
            printf_s("        该级电压损耗：%f %%\n", diss[i]);
            printf_s("        总电压损耗：%f %%\n", diss_acu[i]);
        }*/
    }

    getchar();

    return 0;
}
