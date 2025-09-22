#define SDL_MAIN_HANDLED
#include <iostream>
#include <SDL2/SDL.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <ctime>

SDL_Window* window;
SDL_Renderer* renderer;

using namespace std;

template <typename T>
T clamp(T value,T min,T max) {
    if(value < min) return min;
    if(value > max) return max;
    return value;
}

class vec2 {
public:
    double x,y;
    SDL_Point tosdlpoint() {
        return SDL_Point{(int)round(x),(int)round(y)};
    }
    double m() {
        return y / x; 
    }
    vec2 operator+(const vec2& v) {
        return {x + v.x,y + v.y};
    }
    vec2 operator-(const vec2& v) {
        return {x - v.x,y - v.y};
    }
    vec2 operator*(const double v) {
        return {x * v,y * v};
    }
    vec2 operator/(const double v) {
        return {x/v,y/v};
    }
    vec2 norm() {
        return *this / this->dist();
    }
    vec2 perp() {
        return {-y,x};
    }
    double distSq() {
        return x * x + y * y;
    }
    double dist() {
        return hypot(x,y);
    }
};

double dot(vec2 a,vec2 b) {
    return a.x* b.x + a.y * b.y;
}

vec2 direct(double rad) {
    return {sin(rad),cos(rad)};
}

class triangle {
public:
    vec2 vert;
    double side;
    SDL_Color color;
    static vector<triangle*> triangles;
    triangle(const vec2& v = {0, 0},double s = 1,SDL_Color c = {255,255,255,255}): vert(v),side(s),color(c) {
        triangles.push_back(this);
    }
    void draw() {
        SDL_RenderDrawLine(renderer,vert.x,vert.y,vert.x + side,vert.y);
        SDL_RenderDrawLine(renderer,vert.x,vert.y,vert.x,vert.y + side);
        SDL_RenderDrawLine(renderer,vert.x + side,vert.y,vert.x,vert.y + side);
    }
};

vector<triangle*> triangle::triangles;

const double epsilon = pow(10,-8);

bool monointersection(vec2 origin,vec2 direction,const triangle& t,vec2& pos) {
    double m = (origin.y - direction.y) / (origin.x - direction.x);
    double b = origin.y - m * origin.x;

    vector<vec2> intersections;
    if(abs(m) > epsilon) {
        double x = (t.vert.y - b) / m;
        if(x >= t.vert.x - epsilon && x <= t.vert.x + t.side + epsilon) {
            intersections.push_back(vec2{x, t.vert.y});
        }
    }

    double y = m * t.vert.x + b;
    if(y >= t.vert.y - epsilon && y <= t.vert.y + t.side + epsilon) {
        intersections.push_back({t.vert.x, y});
    }

    double denom = m + 1;
    if(abs(denom) > epsilon) {
        double x = (t.vert.x + t.side + t.vert.y - b) / denom;
        double y = m * x + b;

        bool on_segment =
            x >= t.vert.x - epsilon && x <= t.vert.x + t.side + epsilon &&
            y >= t.vert.y - epsilon && y <= t.vert.y + t.side + epsilon &&
            abs((y + x) - (t.vert.x + t.side + t.vert.y)) < epsilon;

        if(on_segment) {
            intersections.push_back({x, y});
        }
    }
    
    if(intersections.size() > 1 && dot(direction - origin,intersections[0] - origin)>=0 ) {
        pos = intersections[0];
        if((origin - intersections[0]).distSq() > (origin - intersections[1]).distSq()) {
            pos = intersections[1];
        }
        return true;
    }

    return false;
}


bool Cast(vec2 origin,vec2 direction,vec2 &inter,triangle* &obj) {
    vec2 p;
    double min_distSq = INFINITY;
    bool hit = false;

    triangle* tobj;

    double d = 0;
    for(int i = 0; i < triangle::triangles.size(); i++) {
        if(monointersection(origin,direction,*triangle::triangles[i],p)) {
            d = (p - origin).distSq();
            hit = true;
            if(d < min_distSq) {
                min_distSq = d;
                obj = triangle::triangles[i];
                inter = p;
            };
        };
    }
    return hit;
}

void RenderLine(vec2 a,vec2 b) {
    SDL_RenderDrawLine(renderer,a.x,a.y,b.x,b.y);
}

void SetColor(SDL_Color c) {
    SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);
}

SDL_Color ShadeColor(SDL_Color a,SDL_Color b,double w) {
    return SDL_Color{(Uint8)(w*a.r + (1 - w) * b.r),(Uint8)(w*a.g + (1 - w) * b.g),(Uint8)(w*a.b + (1-w)*b.b)};
}

double Rad(double deg) {
    return deg*(M_PI/180);
}

double Deg(double rad) {
    return rad*(180/M_PI);
}

const double w = 1920; const double h = 1080;


vec2 origin = {200,h/2};
vec2 direction = {1,0};
triangle trig1 = {vec2{1000,500},100,{255,0,0,255}};
triangle trig2 = {vec2{900,500},80,{0,255,0,255}};
double m,b;
Uint32 mouseB;
int mouseX,mouseY;
SDL_Event e;

double fov = Rad(60);

double capt_len = 500;
int rays_count = w;
//double view_angle = M_PI;
double Kmoving = 15;

bool mode = true;
bool pause = false;

double renddist = 300;
//double lightshadedist = 70;

chrono::steady_clock::time_point T_start;

double T() {
    chrono::duration<double> elapsed = chrono::high_resolution_clock::now() -T_start;
    return (chrono::duration_cast<chrono::milliseconds>(elapsed).count())/1000.0;
    
}

vec2 ROT_CENTER = {1000,540};
double ROT_RADIUS = 540;

int main() {
    T_start = chrono::high_resolution_clock::now();
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(w,h,0,&window,&renderer);
    vec2 inters;
    int rays_hit;
    //SDL_SetRelativeMouseMode(SDL_TRUE);
	while(1) {
        rays_hit = 0;

        origin = ROT_CENTER;
        origin = origin + vec2{sin(T()) * ROT_RADIUS,cos(T()) * ROT_RADIUS};
        
        direction = (ROT_CENTER - origin).norm();
        
        
        
        // tasti
        while(SDL_PollEvent(&e)) {
            /*if(e.type == SDL_MOUSEMOTION) {
                int dx = e.motion.xrel;
                double sensitivity = -0.1;
                view_angle += dx * sensitivity;

                if(view_angle < 0) view_angle += 360;
                if(view_angle >= 360) view_angle -= 360;
            }
            if(e.key.keysym.scancode == SDL_SCANCODE_W) {
                origin.x += sin(view_angle * deg) * Kmoving;
                origin.y += cos(view_angle * deg) * Kmoving;
            }
            if(e.key.keysym.scancode == SDL_SCANCODE_S) {
                origin.x -= sin(view_angle * deg) * Kmoving;
                origin.y -= cos(view_angle * deg) * Kmoving;
            }
            if(e.key.keysym.scancode == SDL_SCANCODE_A) {
                origin.x += sin((view_angle + 90) * deg) * Kmoving;
                origin.y += cos((view_angle + 90) * deg) * Kmoving;
            }
            if(e.key.keysym.scancode == SDL_SCANCODE_D) {
                origin.x += sin((view_angle - 90) * deg) * Kmoving;
                origin.y += cos((view_angle - 90) * deg) * Kmoving;
            }*/
            if(e.key.keysym.scancode == SDL_SCANCODE_1) {
                mode = false;
            }
            if(e.key.keysym.scancode == SDL_SCANCODE_2) {
                mode = true;
            }
            while(pause) {
                SDL_PollEvent(&e);
                if(e.key.keysym.scancode == SDL_SCANCODE_SPACE && e.key.state == SDL_PRESSED) {
                    pause = !pause;
                }
            }
            if(e.key.keysym.scancode == SDL_SCANCODE_SPACE && e.key.state == SDL_PRESSED) {
                pause = !pause;
            }
        };


        mouseB = SDL_GetMouseState(&mouseX,&mouseY);
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer,0,0,0,255);


        if(mode) {
            if(mouseB == SDL_BUTTON_LEFT) {
                origin = {(double)mouseX,(double)mouseY};
            }
            for(int i = 0; i < triangle::triangles.size(); i++) {
                triangle::triangles[i]->draw();
            }
        }

        
        triangle* objhit;
        int Wt = 0;
        for(double XX = capt_len/2; XX > -(capt_len / 2); XX -= capt_len / rays_count) {
            vec2 ray_origin = origin + direction.perp() * XX;
            
            if(mode) {
                SDL_SetRenderDrawColor(renderer,255,255,255,255);
                SDL_RenderDrawPoint(renderer,ray_origin.x,ray_origin.y);
            }

            if(Cast(ray_origin,ray_origin + direction,inters,objhit)) {
                rays_hit++;
                if(mode) {
                    SDL_SetRenderDrawColor(renderer,255,0,0,255);
                    SDL_RenderDrawPoint(renderer,inters.x,inters.y);
                }
                else {

                    double dist = (ray_origin - inters).dist();
                    double norm = renddist/dist;
                    if(norm > 0.1) {
                        SetColor(ShadeColor(objhit->color,SDL_Color{0,0,0,255},(norm*norm*norm)));
                        SDL_RenderDrawLine(renderer,w - Wt,h / 2 + (h / 2) * norm,w - Wt,h / 2 - (h / 2) * norm);
                    }
                }
            }
            Wt+=w/rays_count;
        }
        if(mode) {
            SDL_SetRenderDrawColor(renderer,255,0,0,255);
            RenderLine(origin,origin + direction * 100);
        }

		SDL_RenderPresent(renderer);
	}
}